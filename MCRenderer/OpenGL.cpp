#pragma once
#ifndef OPEN_GL_C
#define OPEN_GL_C
#include "RegionLoader.h"
#include "Asset.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "OpenGL.h"
#include "Shader.h"
#include <vector>
#include <filesystem>
#include <algorithm>


using namespace std;
using namespace std::filesystem;
using namespace glm;



float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame


////////////////////////////////////


void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouseCursorPossCallback(GLFWwindow* window, double xpos, double ypos)
{
	//cam.mouseInput(xpos, ypos, true);
}

void processInput(GLFWwindow* window, Camera& cam)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		//printf("W");
		cam.keyboardInput(Direction::FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		//printf("S");
		cam.keyboardInput(Direction::BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		//printf("A");
		cam.keyboardInput(Direction::LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		//printf("D");
		cam.keyboardInput(Direction::RIGHT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		//printf("D");
		cam.keyboardInput(Direction::DOWN, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		//printf("D");
		cam.keyboardInput(Direction::UP, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		//printf("E");
		cam.keyboardInput(Direction::YAW_RIGHT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		//printf("Q");
		cam.keyboardInput(Direction::YAW_LEFT, deltaTime);
	}
}


OpenGL::OpenGL(int x, int y)
{
	layerCount = 0;
	EBO = 0;
	VBO = 0;
	VAO = 0;
	screenX = x;
	screenY = y;
	cam = Camera(vec3(16, 75, 16), vec3(0, 1, 0), 0, 0, 10, 1, 1);
	window = glfwCreateWindow(x, y, "Title Goes here", NULL, NULL);
}

OpenGL::~OpenGL()
{
}


vec3 adjust(const float& x, const float& y, const float& z)
{
	return fvec3(( x / 16.0f), (y / 16.0f), (z / 16.0f));
}
float OpenGL::normalizeTexture(const int& texID)
{
	return (float) std::max(0, std::min(layerCount - 1, (int)floor(texID + 0.5f)));
}


vector<VertToBeRendered> OpenGL::convertWorldToVerts(const vector<culledModel>& culledWorld)
{

	printf("converting to verts\n");

	vec3 ppp;
	vec3 ppn;
	vec3 pnp;
	vec3 pnn;
	vec3 npp;
	vec3 npn;
	vec3 nnp;
	vec3 nnn;

	vector<VertToBeRendered> verts;
	for (culledModel cm: culledWorld)//for each block that might exist
	{
		vec2 rotation = vec2(cm.m.xRot, cm.m.yRot);
		mat4 rm = mat4(1.0f);//rm stands for rotation matrix
		//if (cm.m.xRot > 0 || cm.m.yRot > 0)
		//{
		//	printf("rotating %s by %i in x and %i in y\n", cm.m.model.c_str(), cm.m.xRot, cm.m.yRot);
		//}
//TODO: block rotations F in chat
		//rm = rotate(rm, (float)radians((float)cm.m.xRot), vec3(1, 0, 0));
		//rm = rotate(rm, (float)radians((float)cm.m.yRot), vec3(0, 1, 0));

		for (Element e : cm.m.elements)//for each element of that block that might exist
		{
			ppp = (rm * vec4(adjust(e.to.x, e.to.y, e.to.z), 0.0f)) + vec4(cm.coords, 1.0f);
			ppn = (rm * vec4(adjust(e.to.x, e.to.y, e.from.z), 0.0f)) + vec4(cm.coords, 1.0f);
			pnp = (rm * vec4(adjust(e.to.x, e.from.y, e.to.z), 0.0f)) + vec4(cm.coords, 1.0f);
			pnn = (rm * vec4(adjust(e.to.x, e.from.y, e.from.z), 0.0f)) + vec4(cm.coords, 1.0f);
			npp = (rm * vec4(adjust(e.from.x, e.to.y, e.to.z), 0.0f)) + vec4(cm.coords, 1.0f);
			npn = (rm * vec4(adjust(e.from.x, e.to.y, e.from.z), 0.0f)) + vec4(cm.coords, 1.0f);
			nnp = (rm * vec4(adjust(e.from.x, e.from.y, e.to.z), 0.0f)) + vec4(cm.coords, 1.0f);
			nnn = (rm * vec4(adjust(e.from.x, e.from.y, e.from.z), 0.0f)) + vec4(cm.coords, 1.0f);

			//TODO: if anywhere, texture rotations probably go here. IDK how that is to be done yet.
			//todo: probably somethin gto do with the UV i guess.
			//so each of these are the rotated coords of the corners of the cuboid
			//todo: now you can decide whether each face/triangle pair should be there
				
			if (!(cm.faces & 0b00100000))//+y
			{
				float tex = normalizeTexture(e.up.texture);
				verts.push_back(VertToBeRendered( ppp, vec2(1, 1), tex ));
				verts.push_back(VertToBeRendered( ppn, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( npn, vec2(0, 0), tex ));
				verts.push_back(VertToBeRendered( ppp, vec2(1, 1), tex ));
				verts.push_back(VertToBeRendered( npn, vec2(0, 0), tex ));
				verts.push_back(VertToBeRendered( npp, vec2(0, 1), tex ));
			}
			if (!(cm.faces & 0b00010000))//-y
			{
				float tex = normalizeTexture(e.down.texture);
				verts.push_back(VertToBeRendered( pnn, vec2(1, 1), tex ));
				verts.push_back(VertToBeRendered( pnp, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( nnn, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( nnn, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered(pnp, vec2(1, 0), tex));
				verts.push_back(VertToBeRendered( nnp, vec2(0, 0), tex ));
			}
			if (!(cm.faces & 0b00001000))//+x
			{
				float tex = normalizeTexture(e.east.texture);
				verts.push_back(VertToBeRendered( ppp, vec2(0, 0), tex ));
				verts.push_back(VertToBeRendered( pnp, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( ppn, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( ppn, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( pnp, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( pnn, vec2(1, 1), tex ));
			}
			if (!(cm.faces & 0b00000100))//-x
			{
				float tex = normalizeTexture(e.west.texture);
				verts.push_back(VertToBeRendered( npn, vec2(0, 0), tex ));
				verts.push_back(VertToBeRendered( nnn, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( npp, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( npp, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( nnn, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( nnp, vec2(1, 1), tex ));
			}
			if (!(cm.faces & 0b00000010))//+z
			{
				float tex = normalizeTexture(e.south.texture);
				verts.push_back(VertToBeRendered( npp, vec2(0, 0), tex ));
				verts.push_back(VertToBeRendered( pnp, vec2(1, 1), tex ));
				verts.push_back(VertToBeRendered( ppp, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( npp, vec2(0, 0), tex ));
				verts.push_back(VertToBeRendered( nnp, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( pnp, vec2(1, 1), tex ));
			}
			if (!(cm.faces & 0b00000001))//-z
			{
				float tex = normalizeTexture(e.north.texture);
				verts.push_back(VertToBeRendered( ppn, vec2(0, 0), tex ));
				verts.push_back(VertToBeRendered( pnn, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( npn, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( npn, vec2(1, 0), tex ));
				verts.push_back(VertToBeRendered( pnn, vec2(0, 1), tex ));
				verts.push_back(VertToBeRendered( nnn, vec2(1, 1), tex ));
			}
		}
	}
	printf("converted to verts\n");
	return verts;
}

unordered_map<string, int> OpenGL::loadTextures(string path)
{
	unordered_map<string, int> toReturn;
	directory_iterator textureDir(path + "block/");
	vector<pair<string, unsigned char*>> sixteenImages;

	for (auto textureFile : textureDir)
	{
		if (textureFile.path().extension().u8string() == ".png")
		{
			int width, height, channels;

			//TODO: Come back here and figure out animated textures. For now i'm just taking the top most frame and using that statically.
			//todo maybe each one will be a mipmap in each level?
			unsigned char* initImage = stbi_load(textureFile.path().u8string().c_str(), &width, &height, &channels, 0);

			if (initImage == NULL)
			{
				fprintf(stderr, "image not loaded\n");
				fprintf(stderr, "%s\n", textureFile.path().u8string().c_str());
				exit(-1);
			}
			//so the number 1024 might seem like a magic number, but let me explain
			//16pixels*16pixels*4channels(bytes) = 1024, because all the textures come in vertical
			//The images are loaded in row major order, so this works
			unsigned char* cutImage = (unsigned char*)malloc(1024);
			if (cutImage == NULL)
			{
				fprintf(stderr, "memcpy error #5\n");
				exit(-1);
			}
			else
			{
				memcpy(cutImage, initImage, 1024);//this shoulllld be safe
			}
			sixteenImages.push_back({ textureFile.path().filename().stem().u8string(), cutImage });
		}
	}

	printf("all images in sixteenImages\n");
	int32_t mipLevelCount = 1;
	int32_t width = 16;
	int32_t height = 16;
	layerCount = sixteenImages.size();

	printf("layerCount: %i\n", layerCount);


	glGenTextures(1, &largeTextureStack);
	glBindTexture(GL_TEXTURE_2D_ARRAY, largeTextureStack);

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, GL_RGBA8, width, height, layerCount);

	for (int32_t i = 0; i < sixteenImages.size(); i++)
	{
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, sixteenImages[i].second);
		toReturn[sixteenImages[i].first] = i;
		stbi_image_free(sixteenImages[i].second);//todo hopefully this doesn't break shit.
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	printf("all textures loaded into the largeTextureStack\n");
	
	return toReturn;
}


void OpenGL::initializeOpenGL()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, 1);
	//glfwWindowHint(GLFW_SAMPLES, 16);
	window = glfwCreateWindow(screenX, screenY, "Title Goes here", NULL, NULL);

	if (window == NULL)
	{
		cout << "Window creation failed" << endl;
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "GLAD init failed" << endl;
		exit(-1);
	}

	shader = Shader("vertShader.glsl", "fragShader.glsl");

	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	GLsizei stride = sizeof(VertToBeRendered);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); //position
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float))); //uv
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float))); //tex layer
	glEnableVertexAttribArray(2);

}


void OpenGL::run(const vector<VertToBeRendered>& verts)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, largeTextureStack);
	glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
	glBindVertexArray(VAO);

	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(VertToBeRendered), verts.data(), GL_STATIC_DRAW);

	shader.setInt("layerCount", layerCount);

	while (!glfwWindowShouldClose(window))
	{
		shader.use();

		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window, cam);
	
		mat4 view = cam.getView();
		//mat4 view = lookAt(vec3(32 + sin(glfwGetTime()/10)*20, 85, 32+cos(glfwGetTime()/10)*20), vec3(32, 60, 32), vec3(0.0f, 1.0f, 0.0f));
		shader.setMatFour("view", view);
		mat4 projection = perspective(radians(70.0f), (float)screenX / (float)screenY, 0.1f, 256.0f);
		shader.setMatFour("projection", projection);

		glDrawArrays(GL_TRIANGLES, 0, verts.size());
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}


#endif