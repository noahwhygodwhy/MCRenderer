//#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdint.h>
#include <algorithm>
#include "OpenGL.h"
#include "json.hpp"

#include <sstream>

#include "Asset.hpp"
#include "gunzip.hpp"
#include "Chunk.h"
#include <utility>
#include "RegionLoader.h"
#include "NBTTags.hpp"
#include "Biomes.h"
#include "Palette.h"

using namespace std;
using namespace std::filesystem;

static int counter = 0;

//returns a list of strings made of parts of the string split along the delimeters
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

//lowercases any alphabetic letter in a string
string lowerCaser(string s)
{
	string toReturn = "";
	
	for (int i = 0; i < s.length(); i++)
	{
		if (s[i] >= 'A' && s[i] <= 'Z')
		{
			toReturn += s[i] - ('A' - 'a');
		}
		else if (s[i] == ' ')
		{
			toReturn += '_';
		}
		else
		{
			toReturn += s[i];
		}
	}
	//printf("turning %s into %s\n", s, toReturn);
	return toReturn;
}

RegionLoader::RegionLoader()
{

}

RegionLoader::~RegionLoader()
{
	
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
		size_t toReturn = (blockStates[firstLongIndex] >> bitIndex % 64)& generateMask(bitWidth);
		return toReturn;
	}
	else //it's in two longs
	{
		size_t bitWidthOfFirstHalf = 0;
		while( (bitIndex / 64) < secondLongIndex)
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

//initZero MUST be initalized to zero
//made for version 1.15. i think 2
//Takes a byte array of decompressed chunk data, and transforms it into what is
//essentially an array of blocks in the datatype of Models
Chunk * RegionLoader::createChunk(vector<unsigned char> * decompressedData)
{
	Chunk * toReturn = new Chunk();
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
	TagList * sections = level->getTag("Sections")->toList();

	for (auto sec : sections->getValues())//for each section
	{
		CompoundTag* section = sec->toCT();

		if (section->values.count("BlockStates") > 0)//if it is a real section
		{
			Section* toAdd = new Section();
			toAdd->y = section->getTag("Y")->toTag<int8_t>()->getValue();
			TagArray<int64_t>* blockStates =  section->getTag("BlockStates")->toTagArray<int64_t>();
			TagList *p = section->getTag("Palette")->toList();

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
			size_t bitWidth = blockStates->getValues().size()/64;//number of longs/64 is how many bits each one takes
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


pair<int32_t, int32_t> parseRegCoords(string filename)
{
	//printf("parsing %s\n", filename);
	vector<string> stuff = split(filename, '.');
	int32_t x = stoi(stuff[1]);
	int32_t z = stoi(stuff[2]);
	//printf("returning %i, %i\n", x, z);
	return pair(x, z);
}

//reads a file into a buffer, decompresses it, loads it into NBT trees, and then Chunks, and then returns a vector of those chunks
Region* RegionLoader::loadRegion(directory_entry filepath)
{
	Region * reg = new Region();
	pair<int32_t, int32_t> coords = parseRegCoords(filepath.path().filename().stem().u8string());
	reg->x = coords.first;
	reg->z = coords.second;

	basic_ifstream<unsigned char> file;
	file.exceptions(ios::failbit | ios::badbit);

	file.open(filepath.path().u8string(), ios_base::in | ios_base::binary);
	
	file.seekg(0, ios::end);
	unsigned int length = file.tellg();
	file.seekg(ios::beg);

	unsigned char* buffer = new unsigned char[length];
	file.read(buffer, length);

	for (int i = 0; i < 1024; i++)//TODO: should go all the way to 1024
	{
		int ol = i * 4; //ol = offset offset, like the offset for the offset in bytes

		chunkLoc location;
		location.offset = (buffer[ol] << 16 | buffer[ol+1] << 8 | buffer[ol+2]); //ignore this warning, it's a safe read because of the way it is, trust me ;)
		location.sectorCount = buffer[ol + 3];

		if (location.offset != 0)
		{
			int sos = (location.offset) * 4096;

			int sectorLength = (buffer[sos+0] << 24 | buffer[sos + 1] << 16 | buffer[sos + 2] << 8 | buffer[sos + 3]);
			int compressionType = buffer[sos + 4];//if i cared about older versions of minecraft world this would matter = false
			if (compressionType > 2)//check for chunk that is waaaay to big and is in a whole separate file :/
			{
				printf("##########DECOMPRESSION WARNING##############\n");
			}

			sos += 5;//this is a magical number, and i don't remember what it means :|
			vector<unsigned char> * decompressedData = new vector<unsigned char>;

			int status = Deflate(&buffer[sos], sectorLength, [&](unsigned char c) {decompressedData->push_back(c); });

			decompressedData->shrink_to_fit();

			Chunk * c = createChunk(decompressedData);
			
			//if (c->x == 0 && c->z == 0)
			//{
			//	for (auto & [lev, sec] : c->sections)
			//	{
			//		for (int x = 0; x < 16; x++)
			//		{
			//			for (int y = 0; y < 16; y++)
			//			{
			//				for (int z = 0; z < 16; z++)
			//				{
			//					printf("block at %i, %i, %i: %s\n", x+(c->x*16), y+(lev*16), z+(c->z*16), sec->blocks[y][z][x].model.c_str());
			//				}
			//			}
			//		}
			//	}
			//}
			//printf("created chunk %i, %i\n", c->x, c->z);
			//if (c != nullptr)
			//{
				//if (c->x < 1 && c->z < 1)//TODO:
			//	{
					reg->chunks[pair(c->x, c->z)] = c;
			//	}
			//}
		}
	}
	printf("returning region with %lli chunks\n", reg->chunks.size());
	return reg;
}
World *RegionLoader::loadWorld(string saveFolder, Asset * thicc)
{	 
	this->ass = thicc;
	World* world = new World();

	directory_iterator textureDir(saveFolder);
	
	for (auto f : textureDir)
	{
		//printf("seeing region file %s\n", f.path().u8string().c_str());
		if (f.path().filename() == "r.0.0.mca")
		{
			printf("loading region file %s\n", f.path().u8string().c_str());
			Region* toAdd = loadRegion(f);
			world->regions[{toAdd->x, toAdd->z}] = toAdd;
		}
	}
	
	//Region* toAdd = loadRegion(saveFolder+"r.-1.-3.mca");
	//world->regions[pair(toAdd->x, toAdd->z)] = toAdd;

	//world.regions[normX][normZ] = toAdd;

	//make sure the folder has a region folder in it
	//go into the region folder
	//for each .mca file, loadChunk it 

	return world;
}

Model RegionLoader::getBlock(const vec3& coord, World* world)
{
	return getBlock(coord.x, coord.y, coord.z, world);
}

////Gets the block at the coords
////If there isn't a block(not air, but NULL), returns a NULL model
Model RegionLoader::getBlock(const int64_t& worldX, const int64_t& worldY, const int64_t& worldZ, World* world)
{
	if (worldY < 0 || worldY > 255)
	{
		Model m;
		return m;
	}

	//printf("testing %i,%i\n", worldX, worldZ);

	int32_t regX = worldX >> 9; //divides by 512, the size of a region
	int32_t regZ = worldZ >> 9;

	int chkX = worldX >> 4;//divides by 16, the size of a chunk
	int chkZ = worldZ >> 4;

	int secY = worldY / 16;

	int actualY = worldY & 0xF;
	int actualX = worldX & 0xF;
	int actualZ = worldZ & 0xF;

	//printf("testing region %i, %i\n", regX, regZ);
	if (world->regions.count({ regX, regZ }) > 0) //if the region exists
	{
		//printf("it exists\n");
		Region* reg = world->regions.at(pair(regX, regZ)); //get ther region
		//printf("testing chunk %i, %i\n", chkX, chkZ);
		if (reg->chunks.count(pair(chkX, chkZ)) > 0)//if the chuunk exists
		{
			//printf("chk exists\n");
			Chunk* chk = reg->chunks.at(pair(chkX, chkZ));//get the chunk
			if (chk->sections.count(secY) > 0)//if the section exists
			{
				Section* sec = chk->sections.at(secY);//get the section
				return sec->blocks[actualY][actualZ][actualX];//get teh blcok in the section
			}
		}
	}
	Model m;
	return m;
}
//
//
uint8_t RegionLoader::getSides(World* world, const vec3& orig, const Model& origBlock)
{
	vec3 posY = orig + vec3( 0, 1, 0 );
	vec3 negY = orig - vec3( 0, 1, 0 );
	vec3 posX = orig + vec3( 1, 0, 0 );
	vec3 negX = orig - vec3( 1, 0, 0 );
	vec3 posZ = orig + vec3( 0, 0, 1 );
	vec3 negZ = orig - vec3( 0, 0, 1 );

	Model posYB = getBlock(posY, world);
	Model negYB = getBlock(negY, world);
	Model posXB = getBlock(posX, world);
	Model negXB = getBlock(negX, world);
	Model posZB = getBlock(posZ, world);
	Model negZB = getBlock(negZ, world);

	uint8_t faces = 0;
	//todo: need to also take into acount origBlock's elements faces cullface i think
	if (posYB.model == "NULL" || !posYB.cullForMe)
	{
		faces |= 0b00100000;
	}
	if (negYB.model == "NULL" || !negYB.cullForMe)
	{
		faces |= 0b00010000;
	}
	if (posXB.model == "NULL" || !posXB.cullForMe)
	{
		faces |= 0b00001000;
	}
	if (negXB.model == "NULL" || !negXB.cullForMe)
	{
		faces |= 0b00000100;
	}
	if (posZB.model == "NULL" || !posZB.cullForMe)
	{
		faces |= 0b00000010;
	}
	if (negZB.model == "NULL" || !negZB.cullForMe)
	{
		faces |= 0b0000001;
	}
	return faces;
}


vec3 convertBiometoColor(World* world, ivec3 biomeCoords, unsigned char* foliageImage, const int& height, const int& width)
{
	if (biomeCoords.y < 0 || biomeCoords.y > 63)
	{
		return vec3(0, 0, 0);
	}

	int64_t regX = biomeCoords.x / 128;
	if (biomeCoords.x < 0) { regX--; }
	int64_t regZ = biomeCoords.z / 512;
	if (biomeCoords.z < 0) { regZ--; }

	int chkX = (biomeCoords.x % 128) / 4;
	int chkZ = (biomeCoords.z % 128) / 4;
	int chkY = biomeCoords.y / 64;
	

	if (world->regions.count({ regX, regZ }) > 0) //if the region exists
	{
		Region* reg = world->regions.at(pair(regX, regZ)); //get ther region
		if (reg->chunks.count(pair(chkX, chkZ)) > 0)//if the chuunk exists
		{
			Chunk* chk = reg->chunks.at(pair(chkX, chkZ));//get the chunk
			int32_t biomeID = chk->biomes[chkY][chkZ][chkX];
			Biome b = biomeData[biomeID];
			vec3 toReturn;
			//TODO: this is just grass colors, not going between foliage and grass.
			toReturn.x = (b.grass && 0xFF0000)>>16;
			toReturn.y = (b.grass && 0x00FF00)>>8;
			toReturn.z = b.grass && 0x0000FF;
			return toReturn;
		}
	}
	return vec3(0, 0, 0);
}

//takes a block, finds the biome it's in, and applies the tint to that biome
vec3 calculateBiomeTint(const vec3& orig, World* world, unsigned char* foliageImage, const int& height, const int& width)
{
	ivec3 origBiome = ivec3(
		orig.x < 0 ? (((int)orig.x) / 4) - 1 : ((int)orig.x) / 4,
		orig.y < 0 ? (((int)orig.y) / 4) - 1 : ((int)orig.y) / 4,
		orig.z < 0 ? (((int)orig.z) / 4) - 1 : ((int)orig.z) / 4);//god i hope integer rounting works correctly, 
	size_t goodResults = 0;
	vec3 toReturn = vec3(0);
	for (size_t y = -4; y < 5; y += 2)
	{
		for (size_t z = -4; z < 5; z += 2)
		{
			for (size_t x = -4; x < 5; x += 2)
			{
				vec3 colorResult = convertBiometoColor(world, vec3(x, y, z), foliageImage, height, width);
				if (colorResult != vec3(0))
				{
					goodResults++;
					toReturn += colorResult;
				}
			}
		}
	}
	toReturn /= goodResults;
	return toReturn;
	//calculate the biome 4x4x4 coords that contain the orig
	//use getBiome function(to be made) to get the biomes two away and four away in each of the 6 directions
	//                ^this will have to take null into acount
	//then for each of these biomes ids, look up the color
	//sum each of the colors and divide by 13 for the average//not 13 flat, 1 + how ever many times getBiome doesn't return 0,0,0, which will happen when the chunk doesn't exist
	//return that color
}
//
//
////culls the world, returning a vector of every block int he world that might be visible
////these blocks have a 8 bit int, of whos lower 6 bits are used, each one a binary indication
////of whether a side should be displayed. null/null/+y/-y/+x/-x/+z/-z
//this function also handles coloring of grass according to biome because of the conveniance in the flow
vector<culledModel> RegionLoader::cullWorld(World* world)
{
	printf("culling world\n");
	vector<culledModel > toReturn;
	
	int width, height, channels;
	unsigned char* foliageImage = stbi_load((TEXTURE_DIR_PATH + "colormap/foliage.png").c_str(), &width, &height, &channels, 0);

	for (const pair<const pair<int32_t, int32_t>, Region*>& reg : world->regions)//for each region in the world
	{
		for (const pair<const pair<int32_t, int32_t>, Chunk*>& chk : reg.second->chunks)//for each chunk in the world
		{
			printf("accessing chunk %li%, %li\n", chk.first.first, chk.first.second);
			for(const pair<int32_t, Section*>& sec: chk.second->sections)//for each section in that chunk
			{
				for (int y = 0; y < 16; y++)
				{
					for (int x = 0; x < 16; x++)
					{
						for (int z = 0; z < 16; z++)
						{
							vec3 orig = vec3(0, 0, 0);
							//orig.x += (reg.second->x << 9);
							//orig.z += (reg.second->z << 9);

							orig.x += (chk.second->x << 4);
							orig.z += (chk.second->z << 4);
							orig.y += (sec.second->y << 4);
							
							orig.x += x;
							orig.y += y;
							orig.z += z;

							Model origB = getBlock(orig, world);
							
							if (origB.model != "NULL")
							{
								uint8_t sides = getSides(world, orig, origB);
								if (sides > 0)
								{
									culledModel toAdd;
									//todo: tinting here
									//for (Element e : origB.elements)
									//{

									//}
									//toAdd = calculateBiomeTint(orig, world, foliageImage, width, height);//TODO: THIS VARIES ON THE tintIndex value :/
									toAdd.coords = orig;
									toAdd.m = origB;
									toAdd.faces = sides;
									toReturn.push_back(toAdd);
								}
							}
						}
					}
				}
			}
		}
	}
	printf("culled woorld\n");
	return toReturn;
}
