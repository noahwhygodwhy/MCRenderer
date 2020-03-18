#include "OpenGL.h"
#include "RegionLoader.h"
#include "Asset.hpp"
#include <filesystem>


using namespace std;
using namespace std::filesystem;


int main(void)
{

	OpenGL ogl(1600, 900);
	ogl.initializeOpenGL();
	unordered_map<string, int> textureMap = ogl.loadTextures(TEXTURE_DIR_PATH);
	Asset *ass = new Asset(textureMap);

	RegionLoader rl;
	World* world = rl.loadWorld("C:\\Users\\noahm\\AppData\\Roaming\\.minecraft\\saves\\Flat Lands\\region\\", ass);
	//World* world = rl.loadWorld("C:\\Users\\noahm\\OneDrive\\Desktop\\Texture Test World\\region\\", ass);

	vector<culledModel> culledWorld = rl.cullWorld(world);

	printf("size of culled model: %lli\n", culledWorld.size());

	vector<VertToBeRendered> verts = ogl.convertWorldToVerts(culledWorld);
	printf("gtg\n");

	printf("size of vert buffer: %lli\n", verts.size());
	ogl.run(verts);
}