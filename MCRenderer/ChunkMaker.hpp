#pragma once

#include <map>
#include "Chunk.h"
#include "ChunkMaker.hpp"
#include "NBTTags.hpp"

using namespace std;

class ChunkMaker
{
public:
	ChunkMaker();
	~ChunkMaker();
	static map<pair<int, int>, Chunk*> makeChunks(map<pair<int, int>, CompoundTag*> worldNBT);
private:

};

ChunkMaker::ChunkMaker()
{
}

ChunkMaker::~ChunkMaker()
{
}