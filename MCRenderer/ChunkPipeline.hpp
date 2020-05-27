#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <map>

#include "Chunk.h"
#include "NBTTags.hpp"

unordered_map<pair<int, int>, CompoundTag> decompress(string saveFolder);

unordered_map<pair<int, int>, Chunk> createChunks(unordered_map<pair<int, int>, CompoundTag>& worldNBT, Asset& ass);

unordered_map<pair<int, int>, Chunk> cullChunks(unordered_map<pair<int, int>, Chunk>& worldChunks);

unordered_map<pair<int, int>, VertexChunk> verticizeChunks(const unordered_map<pair<int, int>, Chunk>& culledChunks);