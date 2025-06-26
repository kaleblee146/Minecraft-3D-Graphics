/**
This application displays a mesh in wireframe using "Modern" OpenGL 3.0+.
The Mesh3D class now initializes a "vertex array" on the GPU to store the vertices
	and faces of the mesh. To render, the Mesh3D object simply triggers the GPU to draw
	the stored mesh data.
We now transform local space vertices to clip space using uniform matrices in the vertex shader.
	See "simple_perspective.vert" for a vertex shader that uses uniform model, view, and projection
		matrices to transform to clip space.
	See "uniform_color.frag" for a fragment shader that sets a pixel to a uniform parameter.
*/
#define _USE_MATH_DEFINES
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <filesystem>
#include <math.h>

#include <windows.h>

#include "AssimpImport.h"
#include "Mesh3D.h"
#include "Object3D.h"
#include "Animator.h"
#include "ShaderProgram.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>


struct Scene {
	ShaderProgram program;
	std::vector<Object3D> objects;
	std::vector<Animator> animators;
};

/**
 * @brief Constructs a shader program that applies the Phong reflection model.
 */
ShaderProgram phongLightingShader() {
	ShaderProgram shader;
	try {
		// These shaders are INCOMPLETE.
		shader.load("shaders/light_perspective.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Constructs a shader program that performs texture mapping with no lighting.
 */
ShaderProgram texturingShader() {
	ShaderProgram shader;
	try {
		shader.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Loads an image from the given path into an OpenGL texture.
 */
Texture loadTexture(const std::filesystem::path& path, const std::string& samplerName = "baseTexture") {
	StbImage i;
	i.loadFromFile(path.string());
	return Texture::loadImage(i, samplerName);
}

/*****************************************************************************************
*  DEMONSTRATION SCENES
*****************************************************************************************/
Scene bunny() {
	Scene scene{ texturingShader() };

	// We assume that (0,0) in texture space is the upper left corner, but some artists use (0,0) in the lower
	// left corner. In that case, we have to flip the V-coordinate of each UV texture location. The last parameter
	// to assimpLoad controls this. If you load a model and it looks very strange, try changing the last parameter.
	auto bunny = assimpLoad("models/bunny_textured.obj", true);
	bunny.grow(glm::vec3(9, 9, 9));
	bunny.move(glm::vec3(0.2, -1, 0));
	
	// Move all objects into the scene's objects list.
	scene.objects.push_back(std::move(bunny));
	// Now the "bunny" variable is empty; if we want to refer to the bunny object, we need to reference 
	// scene.objects[0]

	Animator spinBunny;
	// Spin the bunny 360 degrees over 10 seconds.
	spinBunny.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	
	// Move all animators into the scene's animators list.
	scene.animators.push_back(std::move(spinBunny));

	return scene;
}


/**
 * @brief Demonstrates loading a square, oriented as the "floor", with a manually-specified texture
 * that does not come from Assimp.
 */
Scene marbleSquare() {
	Scene scene{ texturingShader() };

	std::vector<Texture> textures = {
		loadTexture("models/White_marble_03/Textures_2K/white_marble_03_2k_baseColor.tga", "baseTexture"),
	};
	auto mesh = Mesh3D::square(textures);
	auto floor = Object3D(std::vector<Mesh3D>{mesh});
	floor.grow(glm::vec3(5, 5, 5));
	floor.move(glm::vec3(0, -1.5, 0));
	floor.rotate(glm::vec3(-M_PI / 2, 0, 0));

	scene.objects.push_back(std::move(floor));
	return scene;
}

/**
 * @brief Loads a cube with a cube map texture.
 */
Scene cube() {
	Scene scene{ texturingShader() };

	auto cube = assimpLoad("models/cube.obj", true);

	scene.objects.push_back(std::move(cube));

	Animator spinCube;
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(0, 2 * M_PI, 0)));
	// Then spin around the x axis.
	spinCube.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10.0, glm::vec3(2 * M_PI, 0, 0)));

	scene.animators.push_back(std::move(spinCube));

	return scene;
}

/**
 * @brief Constructs a scene of a tiger sitting in a boat, where the tiger is the child object
 * of the boat.
 * @return
 */
Scene lifeOfPi() {
	// This scene is more complicated; it has child objects, as well as animators.
	Scene scene{ texturingShader() };

	auto boat = assimpLoad("models/boat/boat.fbx", true);
	boat.move(glm::vec3(0, -0.7, 0));
	boat.grow(glm::vec3(0.01, 0.01, 0.01));
	auto tiger = assimpLoad("models/tiger/scene.gltf", true);
	tiger.move(glm::vec3(0, -5, 10));
	// Move the tiger to be a child of the boat.
	boat.addChild(std::move(tiger));

	// Move the boat into the scene list.
	scene.objects.push_back(std::move(boat));

	// We want these animations to referenced the *moved* objects, which are no longer
	// in the variables named "tiger" and "boat". "boat" is now in the "objects" list at
	// index 0, and "tiger" is the index-1 child of the boat.
	Animator animBoat;
	animBoat.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0], 10, glm::vec3(0, 2 * M_PI, 0)));
	Animator animTiger;
	animTiger.addAnimation(std::make_unique<RotationAnimation>(scene.objects[0].getChild(1), 10, glm::vec3(0, 0, 2 * M_PI)));

	// The Animators will be destroyed when leaving this function, so we move them into
	// a list to be returned.
	scene.animators.push_back(std::move(animBoat));
	scene.animators.push_back(std::move(animTiger));

	// Transfer ownership of the objects and animators back to the main.
	return scene;
}

/**
 * Object3D references for sun, creeper, steve, pig
 * empty for now
 */
Object3D* creeperRef = nullptr;
Object3D* sunRef = nullptr;
Object3D* steveRef = nullptr;
Object3D* pigRef = nullptr;

float creeperYaw = 0.0f; // allows creeper to face forward

Scene minecraftScene() {
	Scene scene{ texturingShader() };

	auto cobbleTex = loadTexture("models/Minecraft/cobblestone.png", "baseTexture");
	auto tileMesh = Mesh3D::square({ cobbleTex });

	int gridSize = 100; // 100 tiles of CobbleStone!
	float spacing = 1.0f; // spacing between tiles

	// 2D grid to load tiles, divides by 2 to split between directions for each tile -50, +50
	for (int x = -gridSize / 2; x <= gridSize / 2; ++x) { // iterates left to right for grid
		for (int z = -gridSize / 2; z <= gridSize / 2; ++z) { // iterates through z (front and back)
			Object3D tile({ tileMesh });
			tile.move(glm::vec3(x * spacing, -1.5f, z * spacing));
			tile.rotate(glm::vec3(-M_PI / 2, 0, 0));  // Make it flat
			scene.objects.push_back(std::move(tile));
		}
	}

	// Load Creeper
	auto creeper = assimpLoad("models/Minecraft/Creeper.gltf", true);
	creeper.setName("Creeper"); // set name so memory pointer later
	creeper.move(glm::vec3(0.0f, 0.0f, 0.0f));
	creeper.grow(glm::vec3(1.5f));


	// load Steve
	auto steve = assimpLoad("models/Minecraft/Steve/Steve.gltf", true);
	steve.grow(glm::vec3(0.1f)); // size
	steve.move(glm::vec3(0.0f, 0.0f, 6.0f));
	steve.setOrientation(glm::vec3(0.0f, M_PI, 0.0f)); // turns Steve away from the creeper (for his safety), needs to be in radians (180 degrees)
	scene.objects.push_back(std::move(steve));
	steveRef = &scene.objects.back();  // used to save his movement

	// load pig
	auto pig = assimpLoad("models/Minecraft/Pig/pig.gltf", true);
	pig.grow(glm::vec3(0.1f));
	pig.move(glm::vec3(0.0f, 0.0f, -6.0f));
	pig.setOrientation(glm::vec3(0.0f, M_PI, 0.0f)); // turns pig away from creeper
	scene.objects.push_back(std::move(pig));
	pigRef = &scene.objects.back(); // save pig movement

	Object3D sky(std::vector<Mesh3D>{}); // sky parent node
	auto cloud = assimpLoad("models/Minecraft/Clouds/cloud.gltf", true); // load cloud
	auto sun = assimpLoad("models/Minecraft/sun.gltf", true); // load sun
	sun.move(glm::vec3(-30.0f, 40.0f, -20.0f));  // Sun positioning
	sun.grow(glm::vec3(0.3f)); // Sun size

	cloud.move(glm::vec3(-25.0f, 42.0f, -22.0f));  // Close to sun's position
	cloud.grow(glm::vec3(1.5f));

	sky.addChild(std::move(sun)); // child
	sky.addChild(std::move(cloud)); // child

	scene.objects.push_back(std::move(sky));

	sunRef = &scene.objects.back().getChild(0);
	scene.objects.push_back(std::move(creeper));

	creeperRef = &scene.objects.back();  // creeper position/movement pointer (last object)
	return scene;
}

glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 5.0f); // camera pos in world space
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // scene center
glm::vec3 cameraFront = glm::normalize(cameraTarget - cameraPos); // view direction, (forward)
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); // upward direction
float cameraSpeed = 2.5f;

float yaw = -90.0f;   // starts facing the -Z axis (looking forward)
float pitch = 0.0f;   // horizontal without tilting up or down
float sensitivity = glm::radians(180.0f); // must be in radians not degrees, controls how fast camera rotates

float sunTime = 0.0f;  // used to track the sun time of day
glm::vec3 ambientColor(1.0f); // starts fully white


const float mapMinX = -50.0f; // X min-boundary for map (left)
const float mapMaxX =  50.0f; // X max-boundary for map (right)
const float mapMinZ = -50.0f; // Z-min-boundary for map (down)
const float mapMaxZ =  50.0f; // Z-max boundary for map (up)


glm::vec3 pigFleeDir = glm::vec3(1.0f, 0.0f, 0.0f); // starts the direction that the pig is going
glm::vec3 steveFleeDir = glm::vec3(-1.0f, 0.0f, 0.0f); // starts direction that steve is going






int main() {
	
	std::cout << std::filesystem::current_path() << std::endl;
	
	// Initialize the window and OpenGL.
	sf::ContextSettings settings;
	settings.depthBits = 24; // Request a 24 bits depth buffer
	settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	settings.antialiasingLevel = 2;  // Request 2 levels of antialiasing
	settings.majorVersion = 3;
	settings.minorVersion = 3;
	sf::Window window(sf::VideoMode{ 1200, 800 }, "Modern OpenGL", sf::Style::Resize | sf::Style::Close, settings);

	gladLoadGL();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);  // To see the inside of objects


	// Inintialize scene objects.
	auto myScene = minecraftScene();
	// You can directly access specific objects in the scene using references.
	auto& firstObject = myScene.objects[0];

	// Activate the shader program.
	myScene.program.activate();

	// Set up the view and projection matrices.
	
	// Ready, set, go!
	bool running = true;
	sf::Clock c;
	auto last = c.getElapsedTime();

	// Start the animators.
	for (auto& anim : myScene.animators) {
		anim.start();
	}




	while (running) {
		
		sf::Event ev;
		while (window.pollEvent(ev)) {
			if (ev.type == sf::Event::Closed) {
				running = false;
			}
		}
		auto now = c.getElapsedTime();
		auto diff = now - last;
		std::cout << 1 / diff.asSeconds() << " FPS " << std::endl;
		last = now;

		float deltaTime = diff.asSeconds();

		sunTime += deltaTime; // deltaTime keeps track of how many seconds passed

		if (sunTime < 30.0f) {
			float t = sunTime / 30.0f; // 30 seconds passed
			ambientColor = glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.6f, 0.2f), t); // use mix to blend white to orange
		}
		else if (sunTime < 60.0f) { // 60 seconds passed
			float t = (sunTime - 30.0f) / 30.0f;
			ambientColor = glm::mix(glm::vec3(1.0f, 0.6f, 0.2f), glm::vec3(0.05f, 0.05f, 0.1f), t); // mixes to dark blue/purple (night)
		}
		else if (sunTime < 90.0f) { // 90 seconds passed
			float t = (sunTime - 60.0f) / 30.0f;
			ambientColor = glm::mix(glm::vec3(0.05f, 0.05f, 0.1f), glm::vec3(1.0f, 1.0f, 1.0f), t); // mixes from dark to white
		}
		else {
			sunTime = 0.0f; // restarts the cycle
		}


		if (sunRef) {

			float x = -30.0f + (sunTime / 60.0f) * 60.0f; // Move sun from left (-30) to right (+30) across the sky over 60 seconds
			if (x > 30.0f) {
				x = 1000.0f; // hides the sun after more than 30 seconds
			}

			glm::vec3 sunPos = glm::vec3(x, 40.0f, -20.0f); // fixed height
			sunRef->setPosition(sunPos); // moves it horizontally across the sky
		}





		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { // looks upward
			pitch += sensitivity * deltaTime; // tilts it vertically
			if (pitch > glm::radians(89.0f)) pitch = glm::radians(89.0f); // stops it from going upside down

		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { // looks downward
			pitch -= sensitivity * deltaTime; // decreases the pitch
			if (pitch < glm::radians(-89.0f)) pitch = glm::radians(-89.0f); // capped as well to prevent it from going downside up

		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { // left
			yaw -= sensitivity * deltaTime; // updates the yaw for left turning
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { // right
			yaw += sensitivity * deltaTime; // updates the yaw for right turning
		}

		if (creeperRef) {
			glm::vec3 creeperPos = creeperRef->getPosition(); // gets Creeper position

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
				glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z)); // move based on camera view
				creeperRef->move(horizontalFront * deltaTime * 2.0f);
				creeperYaw = atan2(horizontalFront.x, horizontalFront.z); // get angle to face direction its going
				creeperRef->setOrientation(glm::vec3(0.0f, creeperYaw, 0.0f)); // sets the orientation to face where it is moving
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
				glm::vec3 horizontalBack = glm::normalize(glm::vec3(-cameraFront.x, 0.0f, -cameraFront.z));
				creeperRef->move(horizontalBack * deltaTime * 2.0f); // opposite direction
				creeperYaw = atan2(horizontalBack.x, horizontalBack.z);
				creeperRef->setOrientation(glm::vec3(0.0f, creeperYaw, 0.0f));
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
				glm::vec3 horizontalLeft = glm::normalize(glm::cross(cameraUp, cameraFront));  // cross product
				horizontalLeft.y = 0.0f;
				horizontalLeft = glm::normalize(horizontalLeft);
				creeperRef->move(horizontalLeft * deltaTime * 2.0f); // moves creeper left
				creeperYaw = atan2(horizontalLeft.x, horizontalLeft.z);
				creeperRef->setOrientation(glm::vec3(0.0f, creeperYaw, 0.0f));
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
				glm::vec3 horizontalRight = glm::normalize(glm::cross(cameraFront, cameraUp));  // cross product
				horizontalRight.y = 0.0f;
				horizontalRight = glm::normalize(horizontalRight);
				creeperRef->move(horizontalRight * deltaTime * 2.0f);
				creeperYaw = atan2(horizontalRight.x, horizontalRight.z);
				creeperRef->setOrientation(glm::vec3(0.0f, creeperYaw, 0.0f));
			}


			glm::vec3 direction;
			direction.x = cos(yaw) * cos(pitch); // (left and right)
			direction.y = sin(pitch); // up and down
			direction.z = sin(yaw) * cos(pitch); // forward/backward
			cameraFront = glm::normalize(direction);


			glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z)); // places camera behind creeper
			glm::vec3 offset = -horizontalFront * 5.0f + glm::vec3(0.0f, 3.0f, 0.0f); // 5 units behind, 3 units above creeper
			cameraPos = creeperPos + offset; // places the camerapos to creepers current position


		}

		if (creeperRef && steveRef) {
			glm::vec3 stevePos = steveRef->getPosition(); // get steve position

			glm::vec3 proposedPos = stevePos + steveFleeDir * deltaTime * 1.0f;

			if (proposedPos.x < mapMinX || proposedPos.x > mapMaxX) { // makes sure steve doesn't go off the map
				steveFleeDir.x *= -1.0f;
			}
			if (proposedPos.z < mapMinZ || proposedPos.z > mapMaxZ) {
				steveFleeDir.z *= -1.0f;
			}

			proposedPos = stevePos + steveFleeDir * deltaTime * 1.0f;
			steveRef->setPosition(proposedPos); // updates position
			steveRef->setOrientation(glm::vec3(0.0f, atan2(steveFleeDir.x, steveFleeDir.z), 0.0f)); // updated direction and rotation

			float dist = glm::length(creeperRef->getPosition() - steveRef->getPosition());
			if (dist < 0.8f) { // "explode"

				myScene.objects.erase(std::remove_if(myScene.objects.begin(), myScene.objects.end(),
					[](const Object3D& obj) {
						return &obj == steveRef; // searchs objects and removes Steve's pointer
					})
					, myScene.objects.end());

				steveRef = nullptr; // steve is empty
				for (auto& obj : myScene.objects) {
					if (obj.getName() == "Creeper") {
						creeperRef = &obj; // relink creeper after deleting steve or pig
						break;
					}
				}
			}
		}


		if (creeperRef && pigRef) {
			glm::vec3 pigPos = pigRef->getPosition();

			glm::vec3 proposedPos = pigPos + pigFleeDir * deltaTime * 1.0f;

			// Bounce off walls
			if (proposedPos.x < mapMinX || proposedPos.x > mapMaxX) {
				pigFleeDir.x *= -1.0f;
			}
			if (proposedPos.z < mapMinZ || proposedPos.z > mapMaxZ) {
				pigFleeDir.z *= -1.0f;
			}

			// Recompute pig's movement using possibly updated direction
			proposedPos = pigPos + pigFleeDir * deltaTime * 1.0f;
			pigRef->setPosition(proposedPos);
			pigRef->setOrientation(glm::vec3(0.0f, atan2(pigFleeDir.x, pigFleeDir.z), 0.0f));

			// Creeper explosion if near
			float dist = glm::length(creeperRef->getPosition() - pigRef->getPosition());
			if (dist < 0.8f) {


				myScene.objects.erase(std::remove_if(myScene.objects.begin(), myScene.objects.end(),
					[](const Object3D& obj) { return &obj == pigRef; }), myScene.objects.end());
				pigRef = nullptr;
				for (auto& obj : myScene.objects) {
					if (obj.getName() == "Creeper") {
						creeperRef = &obj;
						break;
					}
				}
			}
		}












		glm::mat4 camera = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // camera position, where its looking, up direction

		glm::mat4 perspective = glm::perspective(
			glm::radians(45.0f),
			static_cast<float>(window.getSize().x) / window.getSize().y,
			0.1f,
			100.0f
		);

		myScene.program.setUniform("view", camera);
		myScene.program.setUniform("projection", perspective);
		myScene.program.setUniform("color", ambientColor); // sets the color


		//myScene.program.setUniform("cameraPos", cameraPos);


		// Update the scene.
		for (auto& anim : myScene.animators) {
			anim.tick(diff.asSeconds());
		}

		// Clear the OpenGL "context".
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(ambientColor.r, ambientColor.g, ambientColor.b, 1.0f); // sets the color for frames



		// Render the scene objects.
		for (auto& o : myScene.objects) {
			o.render(myScene.program);
		}
		window.display();

		if (!steveRef && !pigRef) { // if steve and pig are gone
			Sleep(3);
			running = false; // stop the program
		}

	}

	return 0;
}


