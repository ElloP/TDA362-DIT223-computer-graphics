#include "heightfield.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>

using namespace glm;
using std::string;

HeightField::HeightField(void)
	: m_meshResolution(0)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_numIndices(0)
	, m_texid_hf(UINT32_MAX)
	, m_texid_diffuse(UINT32_MAX)
	, m_heightFieldPath("")
	, m_diffuseTexturePath("")
{
}

void HeightField::loadHeightField(const std::string &heigtFieldPath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	float * data = stbi_loadf(heigtFieldPath.c_str(), &width, &height, &components, 1);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << heigtFieldPath << ".\n";
		return;
	}

	if (m_texid_hf == UINT32_MAX) {
		glGenTextures(1, &m_texid_hf);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data); // just one component (float)

	m_heightFieldPath = heigtFieldPath;
	std::cout << "Successfully loaded heigh field texture: " << heigtFieldPath << ".\n";
}

void HeightField::loadDiffuseTexture(const std::string &diffusePath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	uint8_t * data = stbi_load(diffusePath.c_str(), &width, &height, &components, 3);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << diffusePath << ".\n";
		return;
	}

	if (m_texid_diffuse == UINT32_MAX) {
		glGenTextures(1, &m_texid_diffuse);
	}

	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
	glGenerateMipmap(GL_TEXTURE_2D);

	std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}


void HeightField::generateMesh(int tesselation)
{
	//length of the mesh divided by the tesselation
	float triangleSize = 2.0 / tesselation;
	//each quad has 6 indices, nr of quads == tesselation ^ 2
	m_numIndices = 6 * tesselation * tesselation;

	assert(tesselation % 4 == 0 || tesselation == 1);
	// generate a mesh in range -1 to 1 in x and z
	// (y is 0 but will be altered in height field vertex shader)
	float* positions = new float[3 * m_numIndices];
	for (float x = -1.0f; x < 1; x += triangleSize)
		for (float z = -1.0f; z < 1; z += triangleSize)
		{
			static int i = 0;
			positions[i] = x;
			positions[i + 1] = 0;
			positions[i + 2] = z;

			positions[i + 3] = x;
			positions[i + 4] = 0;
			positions[i + 5] = z + triangleSize;

			positions[i + 6] = x + triangleSize;
			positions[i + 7] = 0;
			positions[i + 8] = z;

			positions[i + 9] = x + triangleSize;
			positions[i + 10] = 0;
			positions[i + 11] = z + triangleSize;

			positions[i + 12] = x + triangleSize;
			positions[i + 13] = 0;
			positions[i + 14] = z;

			positions[i + 15] = x;
			positions[i + 16] = 0;
			positions[i + 17] = z + triangleSize;
			i += 18;
		}
	glGenVertexArrays(1, &m_vao);

	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * m_numIndices, positions, GL_STATIC_DRAW);

	delete[] positions;

	glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void HeightField::submitTriangles(void)
{
	if (m_vao == UINT32_MAX) {
		std::cout << "No vertex array is generated, cannot draw anything.\n";
		return;
	}

	glBindVertexArray(m_vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLES, 0, m_numIndices);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
	glBindVertexArray(0);
}