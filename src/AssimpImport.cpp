#include "AssimpImport.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <unordered_map>



const size_t FLOATS_PER_VERTEX = 3;
const size_t VERTICES_PER_FACE = 3;

std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures) {
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString name;
		mat->GetTexture(type, i, &name);
		std::filesystem::path texPath = modelPath.parent_path() / name.C_Str();
		std::cout << "loading " << texPath << std::endl;

		auto existing = loadedTextures.find(texPath.string());
		if (existing != loadedTextures.end()) {
			textures.push_back(existing->second);
		}
		else {
			StbImage image;
			image.loadFromFile(texPath.string());
			Texture tex = Texture::loadImage(image, typeName);
			textures.push_back(tex);
			loadedTextures.insert(std::make_pair(texPath.string(), tex));
		}
	}
	return textures;
}

Mesh3D fromAssimpMesh(const aiMesh* mesh, const aiScene* scene, const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures) {
	std::vector<Vertex3D> vertices;

	// TODO: fill in this vertices list, by iterating over each element of
	// the mVertices field of the aiMesh pointer. Each element of mVertices
	// has x, y, and z values that you can use to construct a Vertex3D object.
	// To find the u and v texture coordinates of a vertex, access the
	// x and y fields of each element of mTextureCoords.
	// To find the normal vector of a vertex, access the x, y, and z fields
	// of each eleemnt of mNormals.
	for (size_t i = 0; i < mesh->mNumVertices; i++) {

		auto& meshVertex = mesh->mVertices[i];
		auto& texCoord = mesh->mTextureCoords[0][i];
		auto& normal = mesh->mNormals[i];

		Vertex3D vertex(
			meshVertex.x, meshVertex.y, meshVertex.z,
			normal.x, normal.y, normal.z,
			texCoord.x, texCoord.y
			);

		vertices.push_back(vertex);
		// See above.
	}

	std::vector<uint32_t> faces;
	// TODO: fill in the faces list, by iterating over each element of
	// the mFaces field of the aiMesh pointer. Each element of mFaces
	// has an mIndices list, which will have three elements of its own at
	// [0], [1], and [2]. Each of those should be pushed individually onto
	// the faces list.
	for (size_t i = 0; i < mesh->mNumFaces; i++) {
		auto& meshFace = mesh->mFaces[i];
		// See above.
		if (meshFace.mNumIndices == 3) {
			faces.push_back(meshFace.mIndices[0]);
			faces.push_back(meshFace.mIndices[1]);
			faces.push_back(meshFace.mIndices[2]);
		}



	}



	// Load any base textures, specular maps, and normal maps associated with the mesh.
	std::vector<Texture> textures = {};
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<Texture> diffuseMaps = loadMaterialTextures(material,
			aiTextureType_DIFFUSE, "baseTexture", modelPath, loadedTextures);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		std::vector<Texture> specularMaps = loadMaterialTextures(material,
			aiTextureType_SPECULAR, "specMap", modelPath, loadedTextures);
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		std::vector<Texture> normalMaps = loadMaterialTextures(material,
			aiTextureType_HEIGHT, "normalMap", modelPath, loadedTextures);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		normalMaps = loadMaterialTextures(material,
			aiTextureType_NORMALS, "normalMap", modelPath, loadedTextures);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	}

	return Mesh3D(std::move(vertices), std::move(faces), std::move(textures));
}



Object3D assimpLoad(const std::string& path, bool flipTextureCoords) {
	Assimp::Importer importer;

	auto options = aiProcessPreset_TargetRealtime_MaxQuality;
	if (flipTextureCoords) {
		options |= aiProcess_FlipUVs;
	}
	const aiScene* scene = importer.ReadFile(path, options);

	// If the import failed, report it
	if (nullptr == scene) {
		auto* error = importer.GetErrorString();
		std::cerr << "Error loading assimp file: " + std::string(error) << std::endl;
		throw std::runtime_error("Error loading assimp file: " + std::string(error));

	}
	else {

	}
	std::vector<Mesh3D> meshes;
	std::unordered_map<std::string, Texture> loadedTextures;
	auto ret = processAssimpNode(scene->mRootNode, scene, std::filesystem::path(path), loadedTextures);
	return ret;
}

Object3D processAssimpNode(aiNode* node, const aiScene* scene,
	const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures) {

	// Load the aiNode's meshes.
	std::vector<Mesh3D> meshes;
	for (auto i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.emplace_back(fromAssimpMesh(mesh, scene, modelPath, loadedTextures));
	}

	std::vector<Texture> textures;
	for (auto& p : loadedTextures) {
		textures.push_back(p.second);
	}
	glm::mat4 baseTransform;
	for (auto i = 0; i < 4; i++) {
		for (auto j = 0; j < 4; j++) {
			baseTransform[i][j] = node->mTransformation[j][i];
		}
	}
	auto parent = Object3D(std::move(meshes), baseTransform);

	for (auto i = 0; i < node->mNumChildren; i++) {
		Object3D child = processAssimpNode(node->mChildren[i], scene, modelPath, loadedTextures);
		parent.addChild(std::move(child));
	}

	return parent;
}
