#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <map>

#include "Chunk.h"
#include "NBTTags.hpp"

unordered_map<pair<int32_t, int32_t>, CompoundTag*> decompress(string saveFolder);

unordered_map<pair<int32_t, int32_t>, Chunk*> createChunks(unordered_map<pair<int32_t, int32_t>, CompoundTag*>& worldNBT, Asset& ass);

unordered_map<pair<int32_t, int32_t>, Chunk*> cullChunks(unordered_map<pair<int32_t, int32_t>, Chunk*>& worldChunks);

unordered_map<pair<int32_t, int32_t>, VertexChunk*> verticizeChunks(const unordered_map<pair<int32_t, int32_t>, Chunk*>& culledChunks);