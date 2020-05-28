#include "ChunkPipeline.hpp"
#include "gunzip.hpp"

using namespace std;

//splits a string into parts around the delimiter
vector<string> split(const string s, char delim)
{
	vector<string> sections;
	string toAdd;
	istringstream stri(s);
	while (getline(stri, toAdd, delim))
	{
		sections.push_back(toAdd);
	}
	return sections;
}

//gets the region coordinates from the file name
pair<int32_t, int32_t> parseRegCoords(string filename)
{
	//printf("parsing %s\n", filename);
	vector<string> stuff = split(filename, '.');
	int32_t x = stoi(stuff[1]);
	int32_t z = stoi(stuff[2]);
	//printf("returning %i, %i\n", x, z);
	return pair(x, z);
}

//takes the save folder, and decomprempresses the raw chunk data, then parses it into NBT format
//it returns a map where the keys are the global chunk coordinates for the value that is the chunk in NBT format
unordered_map<pair<int32_t, int32_t>, CompoundTag*> decompress(string saveFolder)
{
	unordered_map<pair<int32_t, int32_t>, CompoundTag*> toReturn;// = map<pair<int, int>, CompoundTag*>();
	directory_iterator textureDir(saveFolder);
	for (auto f : textureDir)
	{
		//printf("seeing region file %s\n", f.path().u8string().c_str());

		basic_ifstream<unsigned char> file;
		file.exceptions(ios::failbit | ios::badbit);

		file.open(f.path().u8string(), ios_base::in | ios_base::binary);

		//get the size of the file
		file.seekg(0, ios::end);
		unsigned int length = file.tellg();
		file.seekg(ios::beg);

		unsigned char* buffer = new unsigned char[length]; //TODO unallocate this memory 
		file.read(buffer, length);

		pair<int32_t, int32_t> coords = parseRegCoords(f.path().filename().stem().u8string());
		int regX = coords.first;
		int regZ = coords.second;

		for (int i = 0; i < 1024; i++) //1024 is the number of chunks in a region (32x32)
		{

			int ol = i * 4; //ol = offset offset, like the offset for the offset in bytes

			int offset = (buffer[ol] << 16 | buffer[ol + 1] << 8 | buffer[ol + 2]); //ignore this warning, it's a safe read
			int sectorCount = buffer[ol + 3];

			if (offset != 0)//if the chunk exsists
			{
				int sos = offset * 4096;
				int sectorLength = (buffer[sos + 0] << 24 | buffer[sos + 1] << 16 | buffer[sos + 2] << 8 | buffer[sos + 3]);
				int compressionType = buffer[sos + 4];//if i cared about older versions of minecraft world this would matter = false
				if (compressionType > 2)//check for chunk that is waaaay too big and is in a whole separate file. We're not handling those yet :/ //TODO
				{
					printf("##########DECOMPRESSION WARNING##############\n");
					exit(-1);
				}
				sos += 5;//this is a magical number, and i don't remember exactly what it means. I think it's the offset into the offset section of the file?

				vector<unsigned char> chunkData;
				int status = Deflate(&buffer[sos], sectorLength, [&](unsigned char c) {chunkData.push_back(c); });//decompress
				size_t initZero = 0;
				CompoundTag* chunkNBT = parseNBT(chunkData, &initZero);//convert decompressed byte data to NBT format

				//get the x and z of the chunk
				CompoundTag* root = chunkNBT;//.toCT();
				CompoundTag* level = root->getTag("Level")->toCT();
				int localChunkX = level->getTag("xPos")->toTag<int32_t>()->getValue();
				int localChunkZ = level->getTag("zPos")->toTag<int32_t>()->getValue();
				//and convert it to global chunk coordinates
				int globalChunkX = localChunkX + (regX * 32) + (regX < 0 ? 1 : 0);
				int globalChunkZ = localChunkZ + (regZ * 32) + (regZ < 0 ? 1 : 0);

				toReturn[pair(globalChunkX, globalChunkZ)] = chunkNBT;
			}
		}
	}
	return toReturn;
}

//generates a 64 bit bit mask with <bits> 1s starting from the least most bit
//so if you input 4, the output would be 000000...000000001111
size_t generateMask(const size_t& bits)
{
	size_t toReturn = 0;
	for (int i = 0; i < bits; i++)
	{
		toReturn++;
		toReturn = toReturn << 1;
	}
	toReturn = toReturn >> 1;
	return toReturn;
}

//a function to get a specific block's palette index based on the given
//bitWidth. There's definetly a more efficient way of doing this if it were to be done
//one block after another, but like...it works for now. so i'll put this here 
//todo: make it better
size_t getPaletteID(const vector<int64_t>& blockStates, const size_t& blockIndex, const size_t& bitWidth)
{
	size_t bitIndex = bitWidth * blockIndex;
	size_t firstLongIndex = bitIndex / 64;
	size_t secondLongIndex = (bitIndex + (bitWidth - 1)) / 64;
	if (firstLongIndex == secondLongIndex)//it's all in one long
	{
		size_t toReturn = (blockStates[firstLongIndex] >> bitIndex % 64) & generateMask(bitWidth);
		return toReturn;
	}
	else //it's in two longs
	{
		size_t bitWidthOfFirstHalf = 0;
		while ((bitIndex / 64) < secondLongIndex)
		{
			bitWidthOfFirstHalf++;
			bitIndex++;
		}
		size_t firstHalfValue = (blockStates[firstLongIndex] >> (64 - bitWidthOfFirstHalf)) & generateMask(bitWidthOfFirstHalf);
		size_t secondHalfValue = (blockStates[secondLongIndex] & generateMask(bitWidth - bitWidthOfFirstHalf)) << bitWidthOfFirstHalf;
		size_t toReturn = firstHalfValue | secondHalfValue;
		return toReturn;
	}
}

//Takes the NBT data describing a chunk and returns a Chunk object with the information neatly sorted in it in an easier to read manner.
Chunk* createChunk(CompoundTag* chunkNBT, Asset& ass)
{
	Chunk* toReturn = new Chunk();

	CompoundTag* root = chunkNBT;
	CompoundTag* level = root->getTag("Level")->toCT();

	toReturn->x = level->getTag("xPos")->toTag<int32_t>()->getValue();
	toReturn->z = level->getTag("zPos")->toTag<int32_t>()->getValue();

	//if (toReturn.x != 0 || toReturn.z != 0)//TODO::
	//{
	//	return nullptr;
	//}
	//printf("x and z = %i, %i\n", toReturn.x, toReturn.z);

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
				for (auto& [tagName, tag] : paletteCompound->getValues())//for each element of the item
				{
					if (tagName == "Properties")
					{
						//printf("----\n");
						CompoundTag* properties = tag->toCT();
						for (auto& [subTagName, subTag] : properties->getValues())//for each property
						{
							//printf("subtagName/subtag %s, %s\n", subTagName.c_str(), subTag.toTag<string>().getValue().c_str());
							attributes[subTagName] = subTag->toTag<string>()->getValue();
						}
					}
					if (tagName == "Name")
					{
						name = tag->toTag<string>()->getValue();
						//printf("name: %s\n", name.c_str());
					}
				}
				toAdd->palette.push_back(ass.findModelFromAssets(name, attributes));
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

//takes a map, where keys are global chunk coords, and values are the NBT data describing those chunks, and
//converts it to a map where the keys are global chunk coords and the values are the Chunk objects that describe the chunks
unordered_map<pair<int32_t, int32_t>, Chunk*> createChunks(unordered_map<pair<int32_t, int32_t>, CompoundTag*>& worldNBT, Asset& ass)
{
	unordered_map<pair<int32_t, int32_t>, Chunk*> toReturn;

	for (pair<pair<int, int>, CompoundTag*> entry : worldNBT)
	{
		toReturn[entry.first] = createChunk(entry.second, ass);
	}
	return toReturn;
}

//returns whether the block at the coordinates has the cullForMe flag or not.
bool cullForThisBlock(ivec3 coord, const unordered_map<pair<int32_t, int32_t>, Chunk*>& worldChunks)
{
	pair<int, int> chk = { coord.x >> 4, coord.z >> 4 };
	if (worldChunks.count(chk) > 0) //if the chunk the block would exist in exists
	{
		int sec = coord.y >> 4;
		if (worldChunks.at(chk)->sections.count(sec) > 0)//if the section it would be in exists
		{
			return worldChunks.at(chk)->sections.at(sec)->blocks[coord.y][coord.z][coord.x].cullForMe;
		}
	}
	return false; //nope
}

uint8_t getSides(ivec3 global, const unordered_map<pair<int32_t, int32_t>, Chunk*>& worldChunks)
{
	ivec3 top = global + ivec3(0, 1, 0);
	ivec3 bot = global + ivec3(0, -1, 0);
	ivec3 left = global + ivec3(-1, 0, 0);
	ivec3 right = global + ivec3(10, 0, 0);
	ivec3 back = global + ivec3(0, 0, 1);
	ivec3 front = global + ivec3(0, 0, -1);

	uint8_t toReturn = 0b00000000;
	if (cullForThisBlock(top, worldChunks))
	{
		toReturn &= 0b00100000;
	}
	if (cullForThisBlock(bot, worldChunks))
	{
		toReturn &= 0b00010000;
	}
	if (cullForThisBlock(left, worldChunks))
	{
		toReturn &= 0b00001000;
	}
	if (cullForThisBlock(right, worldChunks))
	{
		toReturn &= 0b00000100;
	}
	if (cullForThisBlock(back, worldChunks))
	{
		toReturn &= 0b00000010;
	}
	if (cullForThisBlock(front, worldChunks))
	{
		toReturn &= 0b00000001;
	}
	return toReturn;
}

//removes all the blocks that wouldn't be visible, and sets what sides should be visible in the model's sides int8
//Returns the same map, just with the modified models in the chunks
unordered_map<pair<int32_t, int32_t>, Chunk*> cullChunks(unordered_map<pair<int32_t, int32_t>, Chunk*>& worldChunks)
{
	for (pair<pair<int, int>, Chunk*> chunk : worldChunks) //for each chunk
	{
		for (pair<int, Section*> section : chunk.second->sections) //for each section in that chunk
		{
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					for (int z = 0; z < 16; z++)
					{
						ivec3 global = ivec3(x, y, z);

						global.x += (chunk.second->x << 4);
						global.z += (chunk.second->z << 4);
						global.y += (section.second->y << 4);

						//Model curr = worldChunks[{chunk.first.first, chunk.first.second}].sections.at(section.first).blocks[y][z][x]; //lol

						worldChunks[{chunk.first.first, chunk.first.second}]->sections.at(section.first)->blocks[y][z][x].faces = getSides(global, worldChunks);

					}
				}
			}
		}
	}
	return worldChunks;
}

unordered_map<pair<int32_t, int32_t>, VertexChunk*> verticizeChunks(const unordered_map<pair<int32_t, int32_t>, Chunk*>& culledChunks)
{
	return unordered_map<pair<int32_t, int32_t>, VertexChunk*>();//TODO
}