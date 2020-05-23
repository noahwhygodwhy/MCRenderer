#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <map>

#include "Chunk.h"
#include "NBTTags.hpp"

map<pair<int, int>, CompoundTag*> decompress(string saveFolder);

map<pair<int, int>, Chunk*> createChunks(map<pair<int, int>, CompoundTag*> worldNBT)