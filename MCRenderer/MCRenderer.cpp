#include <filesystem>
#include <map>
#include <unordered_map>
#include <ctime>

#include "OpenGL.h"
#include "RegionLoader.h"
#include "Asset.hpp"
#include "NBTTags.hpp"
#include "ChunkPipeline.hpp"
#include "Chunk.h"

//#include "windows.h"
//#include "psapi.h"

//#include <windows.h>

using namespace std;
using namespace std::filesystem;


static string saveFolder = "..\\MCRenderer\\GeneralWorld\\region\\";


int main(void)
{

	//PROCESS_MEMORY_COUNTERS_EX pmc;
	//K32GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	//size_t mem =  pmc.PrivateUsage;
	//GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTER*)&pmc, sizeof(pmc));

	OpenGL ogl(1600, 900);
	ogl.initializeOpenGL();
	unordered_map<string, int> textureMap = ogl.loadTextures(TEXTURE_DIR_PATH);
	printf("loading assets\n");
	Asset ass(textureMap);
	printf("initializing openGL\n");
	printf("initialized\n");

	printf("size of chunk: %i\n", sizeof(Chunk));
	printf("size of section: %i\n", sizeof(Section));
	printf("size of vertexChunk: %i\n", sizeof(VertexChunk));
	

	time_t start;

	unordered_map<pair<int32_t, int32_t>, CompoundTag*> worldNBT;
	start = time(0);
	printf("decompressing\n");
	worldNBT = decompress(saveFolder);
	printf("decompressed in %i seconds\n", time(0) - start);

	unordered_map<pair<int32_t, int32_t>, Chunk*> worldChunks;
	start = time(0);
	printf("creating chunks\n");
	worldChunks = createChunks(worldNBT, ass);
	printf("created chunks in %i seconds\n", time(0) - start);

	unordered_map<pair<int32_t, int32_t>, Chunk*> culledChunks;
	start = time(0);
	printf("culling chunks\n");
	culledChunks = cullChunks(worldChunks);
	printf("culled chunks in %i seconds\n", time(0) - start);

	unordered_map<pair<int32_t, int32_t>, VertexChunk*> vertChunks;
	start = time(0);
	printf("verticizing chunks\n");
	vertChunks = verticizeChunks(worldChunks);
	printf("verticized chunks in %i seconds\n", time(0) - start);

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