#include <filesystem>
#include <map>
#include <unordered_map>
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


int main(void)
{


	OpenGL ogl(1600, 900);
	unordered_map<string, int> textureMap = ogl.loadTextures(TEXTURE_DIR_PATH);
	Asset ass(textureMap);
	ogl.initializeOpenGL();

	unordered_map<pair<int32_t, int32_t>, CompoundTag*> worldNBT;// = unordered_map<pair<int, int>, CompoundTag>();
	unordered_map<pair<int32_t, int32_t>, Chunk*> worldChunks;
	unordered_map<pair<int32_t, int32_t>, Chunk*> culledChunks;
	unordered_map<pair<int32_t, int32_t>, VertexChunk*> vertChunks;

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