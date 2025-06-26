#include <iostream>
#include "Mesh3D.h"
#include <glad/glad.h>


Mesh3D::Mesh3D(std::vector<Vertex3D>&& vertices, std::vector<uint32_t>&& faces,
	Texture texture)
	: Mesh3D(std::move(vertices), std::move(faces), std::vector<Texture>{texture}) {
}

Mesh3D::Mesh3D(std::vector<Vertex3D>&& vertices, std::vector<uint32_t>&& faces, std::vector<Texture>&& textures)
	: m_vertexCount(vertices.size()), m_faceCount(faces.size()), m_textures(textures) {

	// Generate a vertex array object on the GPU.
	glGenVertexArrays(1, &m_vao);
	// "Bind" the newly-generated vao, which makes future functions operate on that specific object.
	glBindVertexArray(m_vao);

	// Generate a vertex buffer object on the GPU.
	uint32_t vbo;
	glGenBuffers(1, &vbo);

	// "Bind" the newly-generated vbo, which makes future functions operate on that specific object.
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// This vbo is now associated with m_vao.
	// Copy the contents of the vertices list to the buffer that lives on the GPU.
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D), &vertices[0], GL_STATIC_DRAW);
	// Inform OpenGL how to interpret the buffer: each vertex is 3 floats for position...
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex3D), 0);
	glEnableVertexAttribArray(0);

	// Inform OpenGL how to interpret the buffer: ... then 3 floats for normal vector...
	glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex3D), (void*)12);
	glEnableVertexAttribArray(1);

	// Inform OpenGL how to interpret the buffer: ... the 2 floats for texture coordinate.
	glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex3D), (void*)24);
	glEnableVertexAttribArray(2);


	// Generate a second buffer, to store the indices of each triangle in the mesh.
	uint32_t ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(uint32_t), &faces[0], GL_STATIC_DRAW);

	// Unbind the vertex array, so no one else can accidentally mess with it.
	glBindVertexArray(0);
}

void Mesh3D::addTexture(Texture texture) {
	m_textures.push_back(texture);
}

void Mesh3D::render(ShaderProgram& program) const {
	glBindVertexArray(m_vao);
	for (auto i = 0; i < m_textures.size(); i++) {
		program.setUniform(m_textures[i].samplerName, i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textures[i].textureId);
	}

	// Draw the vertex array, using its "element buffer" to identify the faces.
	glDrawElements(GL_TRIANGLES, m_faceCount, GL_UNSIGNED_INT, nullptr);
	// Deactivate the mesh's vertex array and texture.
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


Mesh3D Mesh3D::square(const std::vector<Texture>& textures) {
	return Mesh3D(
		{
			{ 0.5, 0.5, 0, 0, 0, 1, 1, 0 },    // TR
			{ 0.5, -0.5, 0, 0, 0, 1, 1, 1 },   // BR
			{ -0.5, -0.5, 0, 0, 0, 1, 0, 1 },  // BL
			{ -0.5, 0.5, 0, 0, 0, 1, 0, 0 },   // TL
		},
		{
			2, 1, 3,
			3, 1, 0,
		},
		std::vector<Texture>(textures)
	);
}