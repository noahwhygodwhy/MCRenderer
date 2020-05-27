#include <filesystem>
#include <map>
#include "OpenGL.h"
#include "RegionLoader.h"
#include "Asset.hpp"
#include "NBTTags.hpp"
#include "ChunkPipeline.hpp"
#include "Chunk.h"


//#include <windows.h>

using namespace std;
using namespace std::filesystem;


static string saveFolder = "..\\MCRenderer\\GeneralWorld\\region\\";


struct pairHash
{
public:
	size_t operator ()(const pair<int32_t, int32_t>& value) const
	{
		uint64_t key = ((uint64_t)value.first) << 32 | (uint64_t)value.second;
		key ^= (key >> 33);
		key *= 0xff51afd7ed558ccd;
		key ^= (key >> 33);
		key *= 0xc4ceb9fe1a85ec53;
		key ^= (key >> 33);
		return (size_t)key;
	}
};


struct pairEqualTo
{
	bool operator ()(const pair<int32_t, int32_t>& v1, const pair<int32_t, int32_t>& v2) const
	{
		return (v1.first == v2.first) && (v1.second == v2.second);
	}
};

int main(void)
{


	OpenGL ogl(1600, 900);
	unordered_map<string, int> textureMap = ogl.loadTextures(TEXTURE_DIR_PATH);
	Asset ass(textureMap);
	ogl.initializeOpenGL();

	unordered_map<pair<int, int>, CompoundTag> worldNBT;// = unordered_map<pair<int, int>, CompoundTag>();
	unordered_map<pair<int, int>, Chunk> worldChunks;
	unordered_map<pair<int, int>, Chunk> culledChunks;
	unordered_map<pair<int, int>, VertexChunk> vertChunks;

	worldNBT = decompress(saveFolder);

	worldChunks = createChunks(worldNBT, ass);

	culledChunks = cullChunks(worldChunks);

	vertChunks = verticizeChunks(worldChunks);

	//TODO: see how the performance of frame by frame face gen is, and if it's to slow, this is where you would bake them before sending them to ogl

	//ogl.run(vertChunks);


	

	//RegionLoader rl;
	//World* world = rl.loadWorld("..\\MCRenderer\\GeneralWorld\\region\\", ass);
	////World* world = rl.loadWorld("C:\\Users\\noahm\\OneDrive\\Desktop\\Texture Test World\\region\\", ass);

	//getchar();

	//vector<culledModel> culledWorld = rl.cullWorld(world);

	//printf("size of culled model: %lli\n", culledWorld.size());

	//vector<VertToBeRendered> verts = ogl.convertWorldToVerts(culledWorld);
	//printf("gtg\n");

	//printf("size of vert buffer: %lli\n", verts.size());
	//ogl.run(verts);
}