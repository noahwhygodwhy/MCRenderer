#define _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#pragma once
//#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
#include "NBTTags.hpp"
#include "Chunk.h"
#include <utility>
#include <unordered_map>
#include "Asset.hpp"
#include <filesystem>

using namespace std;
using namespace std::filesystem;
using namespace glm;

typedef struct
{
	int offset : 24;
	int sectorCount : 8;
}chunkLoc;

struct culledModel
{
	uint8_t faces;
	Model m;
	vec3 coords;
};

class RegionLoader
{
public:
	RegionLoader();
	~RegionLoader();
	Region* loadRegion(directory_entry filename);
	Chunk* createChunk(vector<unsigned char>* decompressedData);
	World* loadWorld(string saveFolder, Asset * ass);
	//void cullRegion(Region reg);
	vector<culledModel> cullWorld(World* world);
	Model getBlock(const vec3& coord, World *world);
	Model getBlock(const int64_t& x, const int64_t& y, const int64_t& z, World* world);
	uint8_t getSides(World* world, const vec3& orig, const Model& origBlock);




private:
	Asset *ass;
};
