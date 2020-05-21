#pragma once
#include <map>
#include <vector>
#include <string>

#include "NBTTags.hpp"

using namespace std;

class Decompressor
{
public:
	Decompressor();
	~Decompressor();
	map<pair<int, int>, CompoundTag*> decompress(string saveFolder);
private:

};
