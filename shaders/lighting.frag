#version 330
// A fragment shader for rendering fragments in the Phong reflection model.
layout (location=0) out vec4 FragColor;

// Inputs: the texture coordinates, world-space normal, and world-space position
// of this fragment, interpolated between its vertices.
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragWorldPos;

// Uniforms: MUST BE PROVIDED BY THE APPLICATION.

// The mesh's base (diffuse) texture.
uniform sampler2D baseTexture;

// Material parameters for the whole mesh: k_a, k_d, k_s, shininess.
uniform vec4 material;

// Ambient light color.
uniform vec3 ambientColor;

// Direction and color of a single directional light.
uniform vec3 directionalLight; // this is the "I" vector, not the "L" vector.
uniform vec3 directionalColor;



// Location of the camera.
uniform vec3 viewPos;


void main() {
    // TODO: using the lecture notes, compute ambientIntensity, diffuseIntensity, 
    // and specularIntensity.

   // vec3 ambientIntensity = vec3(0);
    //vec3 diffuseIntensity = vec3(0);
    //vec3 specularIntensity = vec3(0);

    //vec3 lightIntensity = ambientIntensity + diffuseIntensity + specularIntensity;
    //FragColor = vec4(lightIntensity, 1) * texture(baseTexture, TexCoord);

    // Normalize the normal and light direction
        vec3 N = normalize(Normal); // surface normal
        vec3 L = normalize(-directionalLight); // light vector pointing *toward* the surface
        vec3 V = normalize(viewPos - FragWorldPos); // view direction
        vec3 R = reflect(-L, N); // reflected light direction

        // Material components
        float k_a = material.x;
        float k_d = material.y;
        float k_s = material.z;
        float shininess = material.w;

        // 1. Ambient component
        vec3 ambientIntensity = k_a * ambientColor;

        // 2. Diffuse component
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuseIntensity = k_d * NdotL * directionalColor;

        // 3. Specular component (only if facing light)
        float RdotV = max(dot(R, V), 0.0);
        vec3 specularIntensity = k_s * pow(RdotV, shininess) * directionalColor;

        // Combine all components
        vec3 lightIntensity = ambientIntensity + diffuseIntensity + specularIntensity;

        // Multiply by texture color
        vec4 texColor = texture(baseTexture, TexCoord);
        FragColor = texture(baseTexture, TexCoord);  // test just the texture

}