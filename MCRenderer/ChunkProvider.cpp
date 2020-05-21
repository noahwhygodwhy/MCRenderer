#include "ChunkProvider.h"
#include "NBTTags.hpp"
#include "Biomes.h"
#include "Palette.h"

ChunkProvider::ChunkProvider(map<pair<int, int>, vector<unsigned char>> decompressedData)
{
	this->decompressedData = map<pair<int, int>, vector<unsigned char>>();
}

ChunkProvider::~ChunkProvider()
{
}

Chunk* ChunkProvider::getChunk(int chkX, int chkY)
{

	Chunk* toReturn = new Chunk();
	size_t initZero = 0;//IMPORTANT





	CompoundTag* result = parseNBT(decompressedData, &initZero);
	//printf("NBT1 Parsed\n");
	CompoundTag* root = result->toCT();

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
}