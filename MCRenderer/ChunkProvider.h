#pragma once

#include <map>
#include <vector>

#include "NBTTags.hpp"
#include "Chunk.h"

using namespace std;

class ChunkProvider
{
	map<pair<int, int>, CompoundTag*> chunkNBT;
public:
	ChunkProvider(map<pair<int, int>, CompoundTag*> chunkNBT);
	~ChunkProvider();
	Chunk* getChunk(int chkX, int chkY);
private:

};
