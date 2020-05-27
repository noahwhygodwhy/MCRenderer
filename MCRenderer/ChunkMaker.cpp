/*#include "ChunkMaker.hpp"

using namespace std;

//takes a map, where keys are global chunk coords, and values are the NBT data describing those chunks, and
//converts it to a map where the keys are global chunk coords and the values are the Chunk objects that describe the chunks
inline map<pair<int, int>, Chunk*> ChunkMaker::makeChunks(map<pair<int, int>, CompoundTag*> worldNBT)
{
	map<pair<int, int>, Chunk*> toReturn = map<pair<int, int>, Chunk*>();

	for (pair<pair<int, int>, CompoundTag*> entry : worldNBT)
	{
		toReturn[entry.first] = createChunk(entry.second);
	}
	return toReturn;
}

//Takes the NBT data describing a chunk and returns a Chunk object with the information neatly sorted in it in an easier to read manner.
Chunk* createChunk(CompoundTag* chunkNBT)
{
	Chunk* toReturn = new Chunk();

	CompoundTag* root = chunkNBT;
	CompoundTag* level = root->getTag("Level")->toCT();

	toReturn->x = level->getTag("xPos")->toTag<int32_t>()->getValue();
	toReturn->z = level->getTag("zPos")->toTag<int32_t>()->getValue();

	//if (toReturn->x != 0 || toReturn->z != 0)//TODO::
	//{
	//	return nullptr;
	//}
	//printf("x and z = %i, %i\n", toReturn->x, toReturn->z);

	if (level->getValues().count("Biomes") > 1)
	{
		vector<int32_t> biomes = level->getTag("Biomes")->toTagArray<int32_t>()->getValues();
		size_t i = 0;
		for (size_t z = 0; z < 16; z++)
		{
			for (size_t x = 0; x < 16; x++)
			{
				for (size_t y = 0; y < 64; y++)
				{
					toReturn->biomes[y][z][x] = biomes[i++];
				}
			}
		}
	}
	TagList* sections = level->getTag("Sections")->toList();

	for (auto sec : sections->getValues())//for each section
	{
		CompoundTag* section = sec->toCT();

		if (section->values.count("BlockStates") > 0)//if it is a real section
		{
			Section* toAdd = new Section();
			toAdd->y = section->getTag("Y")->toTag<int8_t>()->getValue();
			TagArray<int64_t>* blockStates = section->getTag("BlockStates")->toTagArray<int64_t>();
			TagList* p = section->getTag("Palette")->toList();

			for (auto pc : p->getValues())//for each item in the palette
			{
				CompoundTag* paletteCompound = pc->toCT();
				string name;
				unordered_map<string, string> attributes;
				for (const auto& [tagName, tag] : paletteCompound->getValues())//for each element of the item
				{
					if (tagName == "Properties")
					{
						//printf("----\n");
						CompoundTag* properties = tag->toCT();
						for (const auto& [subTagName, subTag] : properties->getValues())//for each property
						{
							//printf("subtagName/subtag %s, %s\n", subTagName.c_str(), subTag->toTag<string>()->getValue().c_str());
							attributes[subTagName] = subTag->toTag<string>()->getValue();
						}
					}
					if (tagName == "Name")
					{
						name = tag->toTag<string>()->getValue();
						//printf("name: %s\n", name.c_str());
					}
				}
				toAdd->palette.push_back(ass->findModelFromAssets(name, attributes));
			}
			size_t bitWidth = blockStates->getValues().size() / 64;//number of longs/64 is how many bits each one takes
			for (size_t y = 0; y < 16; y++)
			{
				for (size_t z = 0; z < 16; z++)
				{
					for (size_t x = 0; x < 16; x++)
					{
						size_t blockIndex = (y * 256) + (z * 16) + x;
						toAdd->blocks[y][z][x] = toAdd->palette[getPaletteID(blockStates->getValues(), blockIndex, bitWidth)];
					}
				}
			}
			toReturn->sections[toAdd->y] = toAdd;
		}
	}

	return toReturn;
}*/