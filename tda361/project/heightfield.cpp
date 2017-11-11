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

	// generate a mesh in range -1 to 1 in x and z
	// (y is 0 but will be altered in height field vertex shader)
	std::vector<float> positions;
	std::vector<float> texCoords;
	std::vector<unsigned int> indices;
	for (float x = -1.0f; x < 1; x += triangleSize)
		for (float z = -1.0f; z < 1; z += triangleSize)
		{
			positions.push_back(x);
			positions.push_back(0);
			positions.push_back(z);

			texCoords.push_back((x + 1) / 2);
			texCoords.push_back((z + 1) / 2);

			positions.push_back(x);
			positions.push_back(0);
			positions.push_back(z + triangleSize);

			texCoords.push_back((x + 1) / 2);
			texCoords.push_back((z + triangleSize + 1) / 2);

			positions.push_back(x + triangleSize);
			positions.push_back(0);
			positions.push_back(z);

			texCoords.push_back((x + triangleSize + 1) / 2);
			texCoords.push_back((z + 1) / 2);

			positions.push_back(x + triangleSize);
			positions.push_back(0);
			positions.push_back(z + triangleSize);

			texCoords.push_back((x + triangleSize + 1) / 2);
			texCoords.push_back((z + triangleSize + 1) / 2);

			positions.push_back(x + triangleSize);
			positions.push_back(0);
			positions.push_back(z);

			texCoords.push_back((x + triangleSize + 1) / 2);
			texCoords.push_back((z + 1) / 2);

			positions.push_back(x);
			positions.push_back(0);
			positions.push_back(z + triangleSize);

			texCoords.push_back((x + 1) / 2);
			texCoords.push_back((z + triangleSize + 1) / 2);
		} 
	/*
	for (float z = -1.0f; z <= 1; z += triangleSize)
		for (float x = -1.0f; x <= 1; x += triangleSize) 
		{
			positions.push_back(x);
			positions.push_back(0);
			positions.push_back(z);

			texCoords.push_back((x + 1) / 2);
			texCoords.push_back((z + 1) / 2);
		}

	for(int i = 0; i < m_numIndices; i++) 
	{
		if(i % tesselation == 0 && i != 0)
			continue;
		indices.push_back(i);
		indices.push_back(i + 1);
		indices.push_back(i + 1 + tesselation);

		indices.push_back(i + 1);
		indices.push_back(i + 2 + tesselation);
		indices.push_back(i + 1 + tesselation);

	} 
	*/
		

	glGenVertexArrays(1, &m_vao);

	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

	glGenBuffers(1, &m_uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), texCoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, 0);

	/*
	glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);*/

	//delete[] indices;
	//delete[] texCoords;
	//delete[] positions;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(2);
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
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	//glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_TRIANGLES, 0, m_numIndices);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
	glBindVertexArray(0);
}