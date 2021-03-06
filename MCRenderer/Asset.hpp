#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <string_view>
#include "json.hpp"
#include <glm/glm.hpp>

using namespace std;

using namespace nlohmann;

using namespace glm;

static const string TEXTURE_DIR_PATH = "../MCRenderer/minecraft/textures/";
static const string MODEL_DIR_PATH = "../MCRenderer/minecraft/models/";
static const string BLOCKSTATE_DIR_PATH = "../MCRenderer/minecraft/blockstates/";

struct Face
{
	int texture = 0;
	uint8_t cullFace = 0b00000000;
	ivec4 uv = ivec4(0, 0, 16, 16);
	vec3 tint = vec3(0, 0, 0);
	int rotation = 0;//TODO handle texture rotation, not just block rotation
	int tintIndex = -1;
	//todo tint index
};



struct Element
{
	Face down;
	Face up;
	Face north;
	Face south;
	Face west;
	Face east;
	vec3 from = vec3(0, 0, 0);
	vec3 to = vec3(16, 16, 16);
	int xRot = 0;
	int yRot = 0;
	bool uvLock = false;
};

//texture should be a texturearray
//multiple textures at same time
struct Model
{
	vector<Element> elements;
	string model = "NULL";
	//todo: int weight = 1;
	bool AmbOcc = true;
	bool cullForMe = false;
	//todo: particle
};

enum BlockStateType
{
	VARIANT,
	MULTIPART
};

struct Conditions
{
	unordered_map<string, string> conditions;
};
struct Conditional
{
	vector<Conditions> when;//OR'd
	Model model;//"apply"
};
struct BlockState
{
	BlockStateType type;
	vector<Conditional> variants;
};

class Asset
{
	unsigned int largeTextureStack;
	unordered_map<string, int> textureMap;
	unordered_map<string, BlockState> assets;
public:
	Asset(const unordered_map<string, int>& tm);
	~Asset();
	Model findModelFromAssets(string mame, const unordered_map<string, string> &attributes);

private:
	json fileToJson(string filepath);
	Face parseFaceJson(const json &faces,const string& faceStr, const unordered_map<string, string>& textures);
	Model parseModelJson(string name, int xRot, int yRot, int uvLock);
	Model parseModel(const json& j);
	BlockState parseBlockstateJson(string filepath);
	unordered_map < string, string > parseAttributes(string src);
	//unsigned int importTextures(const string& path);


};

//class Attribute
//{
//public:
//	Attribute();
//	~Attribute();
//
//private:
//
//};
//
//Attribute::Attribute()
//{
//}
//
//Attribute::~Attribute()
//{
//}