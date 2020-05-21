#pragma once

#include <map>
#include <vector>

#include "Chunk.h"

using namespace std;

class ChunkProvider
{
	map<pair<int, int>, vector<unsigned char>> decompressedData;
public:
	ChunkProvider(map<pair<int, int>, CompoundTag*> decompressedData);
	~ChunkProvider();
	Chunk* getChunk(int chkX, int chkY);
private:

};
