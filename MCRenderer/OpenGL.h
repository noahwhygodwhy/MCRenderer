#pragma once
#ifndef OPEN_GL_H
#define OPEN_GL_H

#include "Chunk.h"
#include "Asset.hpp"
#include "RegionLoader.h"
#include "glad.h"
#include <GLFW/glfw3.h>
#include "GLSLReader.h"
#include "Shader.h"
#include "Camera.h"
#include <map>
#include <string>
#include <vector>

#include "stb_image.h"

using namespace std;


struct VertToBeRendered
{
	fvec3 coordinates;
	fvec2 uv;
	float texIndex;
	VertToBeRendered(vec3 c, vec2 u, float t)
	{
		coordinates = c;
		uv = u;
		texIndex = t;
	}
};

class OpenGL
{
	unsigned int largeTextureStack = 0;
	int screenX;
	int screenY;
	Camera cam;
	Shader shader;
	GLFWwindow* window;
	unsigned int VBO, VAO, EBO;
	int32_t layerCount;
public:
	OpenGL(int screenX, int screenY);
	~OpenGL();
	void initializeOpenGL();
	void run(const unordered_map<pair<int32_t, int32_t>, vector<Vert>>& vertisizedChunks);
	unordered_map<string, int> loadTextures(string path);
	//vector<Vert> convertWorldToVerts(const vector<culledModel>& culledWorld);
	//void addFace(vector<Vert>& verts, const vec3& a, const vec3& b, const vec3& c, const vec3& d, vec4 uv, int texRotation, int uvRotation, bool uvLock, int texture);

private:
	float normalizeTexture(const int& texID);
};
vec3 adjust(const float& x, const float& y, const float& z);
#endif;
