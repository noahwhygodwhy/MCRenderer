#include "ChunkPipeline.hpp"
#include "gunzip.hpp"
#include "Chunk.h"
#include "OpenGL.h"

using namespace std;
using namespace glm;

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

				if(globalChunkZ >= 0 && globalChunkZ <=15 && globalChunkX >= 0 && globalChunkX <= 15)
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
Chunk* createChunk(CompoundTag* chunkNBT, Asset& ass, pair<int32_t, int32_t> globalCoords)
{
	Chunk* toReturn = new Chunk();

	CompoundTag* root = chunkNBT;
	CompoundTag* level = root->getTag("Level")->toCT();

	toReturn->x = globalCoords.first;
	toReturn->z = globalCoords.second;

	printf("creating Chunk %i, %i\n", toReturn->x, toReturn->z);
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


/*unordered_map<pair<int32_t, int32_t>, Chunk*> createChunks(unordered_map<pair<int32_t, int32_t>, CompoundTag*>& worldNBT, Asset& ass)
{
	unordered_map<pair<int32_t, int32_t>, Chunk*> toReturn;

	for (pair<pair<int, int>, CompoundTag*> entry : worldNBT)
	{
		toReturn[entry.first] = createChunk(entry.second, ass);
	}
	return toReturn;
}*///takes a map, where keys are global chunk coords, and values are the NBT data describing those chunks, and
//converts it to a map where the keys are global chunk coords and the values are the Chunk objects that describe the chunks

//returns whether the block at the coordinates has the cullForMe flag or not.
bool cullForThisBlock(ivec3 coord, Chunk* chk)
{
	//return false; //TODO: 
	if (coord.x >= 0 && coord.x <= 15 && coord.z >= 0 && coord.z <= 15)//if it's still in the chunk
	{
		int sec = coord.y >> 4;
		if (chk->sections.count(sec) > 0)
		{
			return chk->sections.at(sec)->blocks[coord.y%16][coord.z][coord.x].cullForMe;
			
		}
	}
	return false; //nope
}

uint8_t getSides(ivec3 chkRelativeCoord,  Chunk* worldChunks)
{
	ivec3 top = chkRelativeCoord + ivec3(0, 1, 0);
	ivec3 bot = chkRelativeCoord + ivec3(0, -1, 0);
	ivec3 left = chkRelativeCoord + ivec3(-1, 0, 0);
	ivec3 right = chkRelativeCoord + ivec3(1, 0, 0);
	ivec3 back = chkRelativeCoord + ivec3(0, 0, 1);
	ivec3 front = chkRelativeCoord + ivec3(0, 0, -1);

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


//sets the face byte for each block, determining which faces should be translated to triangles  
//TODO: does not handle the removal of faces at the edge of chunks. Idek if i need to do that, will find out
void cullChunk(Chunk* chk)
{
	for (pair<int, Section*> section : chk->sections) //for each section in that chunk
	{
		for (int y = 0; y < 16; y++) //for each y latyer
		{
			for (int x = 0; x < 16; x++)
			{
				for (int z = 0; z < 16; z++)
				{
					ivec3 chkRelativeCoords = ivec3(x, y, z);
					chkRelativeCoords.y += (section.second->y << 4);

					//Model curr = worldChunks[{chunk.first.first, chunk.first.second}].sections.at(section.first).blocks[y][z][x]; //lol

					uint8_t sides = getSides(chkRelativeCoords, chk);
					if (sides > 0)
					{
						section.second->blocks[y][z][x].coords = ivec3(x, y, z) + ivec3(chk->x << 4, section.second->y << 4, chk->z << 4); //sets the coordinates in the global scale
						section.second->blocks[y][z][x].faces = getSides(chkRelativeCoords, chk);
					}
					else
					{
						section.second->blocks[y][z][x].model = "NULL";
					}
				}
			}
		}
	}
}

//removes all the blocks that wouldn't be visible, and sets what sides should be visible in the model's sides int8
//Returns the same map, just with the modified models in the chunks
/*unordered_map<pair<int32_t, int32_t>, Chunk*> cullChunks(unordered_map<pair<int32_t, int32_t>, Chunk*>& worldChunks)
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
}*/

vec2 rot90(vec2 v)
{
	return vec2(v.y, 1.0f - v.x);
}

void addFace(vector<Vert>& verts, const vec3& a, const vec3& b, const vec3& c, const vec3& d, vec4 uv, int texRotation, int uvRotation, bool uvLock, int texture)
{
	vec2 uv00 = vec2(uv.x, uv.y) / 16.0f;
	vec2 uv01 = vec2(uv.x, uv.w) / 16.0f;
	vec2 uv11 = vec2(uv.z, uv.w) / 16.0f;
	vec2 uv10 = vec2(uv.z, uv.y) / 16.0f;

	if (uvLock)
	{
		texRotation -= uvRotation;
	}

	texRotation = ((texRotation % 360) + 360) % 360;//confines to 0 to 360

	for (int i = 0; i < texRotation; i += 90)
	{
		uv00 = rot90(uv00);
		uv01 = rot90(uv01);
		uv11 = rot90(uv11);
		uv10 = rot90(uv10);
	}


	Vert v00(a, uv00, texture);
	Vert v01(b, uv01, texture);
	Vert v11(c, uv11, texture);
	Vert v10(d, uv10, texture);

	verts.push_back(v00);
	verts.push_back(v11);
	verts.push_back(v10);

	verts.push_back(v00);
	verts.push_back(v01);
	verts.push_back(v11);
}

vec3 adjust(const float& x, const float& y, const float& z)
{
	return fvec3((x / 16.0f), (y / 16.0f), (z / 16.0f));
}

vec4 rotateAroundCenter(const mat4& rm, const vec4& b)
{
	return vec4(vec3(0.5f), 0.0f) + (rm * (b - vec4(vec3(0.5f), 0.0f)));
}

//vec4 rotateUV(int degrees, const vec4& b)
//{
//	vec4 f = b;
//	for (int i = 0; i < degrees / 90; i++)
//	{
//		f = vec4(f.z, f.y, f.x, f.w);
//	}
//	return f;
//}


vector<Vert> verticizeChunk(Chunk* chk)
{
	printf("converting to verts\n");

	vec3 ppp;
	vec3 ppn;
	vec3 pnp;
	vec3 pnn;
	vec3 npp;
	vec3 npn;
	vec3 nnp;
	vec3 nnn;

	vector<Vert> verts;

	for (const auto& [secCoord, sec] : chk->sections)
	{
		for (const auto& yLevel : sec->blocks)
		{
			for (const auto& zSlice : yLevel)
			{
				for (const auto& block : zSlice)
				{
					for (Element e : block.elements)
					{
						mat4 rm = mat4(1.0f);//rm stands for rotation matrix
						rm = rotate(rm, (float)radians((float)e.yRot), vec3(0, -1, 0));
						rm = rotate(rm, (float)radians((float)e.xRot), vec3(-1, 0, 0));

						ppp = rotateAroundCenter(rm, vec4(adjust(e.to.x, e.to.y, e.to.z), 0.0f)) + vec4(block.coords, 0.0f);
						ppn = rotateAroundCenter(rm, vec4(adjust(e.to.x, e.to.y, e.from.z), 0.0f)) + vec4(block.coords, 0.0f);
						pnp = rotateAroundCenter(rm, vec4(adjust(e.to.x, e.from.y, e.to.z), 0.0f)) + vec4(block.coords, 0.0f);
						pnn = rotateAroundCenter(rm, vec4(adjust(e.to.x, e.from.y, e.from.z), 0.0f)) + vec4(block.coords, 0.0f);
						npp = rotateAroundCenter(rm, vec4(adjust(e.from.x, e.to.y, e.to.z), 0.0f)) + vec4(block.coords, 0.0f);
						npn = rotateAroundCenter(rm, vec4(adjust(e.from.x, e.to.y, e.from.z), 0.0f)) + vec4(block.coords, 0.0f);
						nnp = rotateAroundCenter(rm, vec4(adjust(e.from.x, e.from.y, e.to.z), 0.0f)) + vec4(block.coords, 0.0f);
						nnn = rotateAroundCenter(rm, vec4(adjust(e.from.x, e.from.y, e.from.z), 0.0f)) + vec4(block.coords, 0.0f);

						if (block.faces & 0b00100000 && !(e.up.cullFace & 0b11000000))//+y
						{
							addFace(verts, npn, npp, ppp, ppn, e.up.uv, e.up.rotation, e.yRot, e.uvLock, e.up.texture);
						}
						if (block.faces & 0b00010000 && !(e.down.cullFace & 0b11000000))//-y
						{
							addFace(verts, nnp, nnn, pnn, pnp, e.down.uv, e.down.rotation, e.yRot, e.uvLock, e.down.texture);
						}
						if (block.faces & 0b00001000 && !(e.east.cullFace & 0b11000000))//+x
						{
							addFace(verts, ppp, pnp, pnn, ppn, e.east.uv, e.east.rotation, e.yRot % 180 == 90 ? 0 : e.xRot, e.uvLock, e.east.texture);
						}
						if (block.faces & 0b00000100 && !(e.west.cullFace & 0b11000000))//-x
						{
							addFace(verts, npn, nnn, nnp, npp, e.west.uv, e.west.rotation, e.yRot % 180 == 90 ? 0 : e.xRot, e.uvLock, e.west.texture);
						}
						if (block.faces & 0b00000010 && !(e.south.cullFace & 0b11000000))//+z
						{
							addFace(verts, npp, nnp, pnp, ppp, e.south.uv, e.south.rotation, e.yRot % 180 == 0 ? 0 : e.xRot, e.uvLock, e.south.texture);
						}
						if (block.faces & 0b00000001 && !(e.north.cullFace & 0b11000000))//-z
						{
							addFace(verts, ppn, pnn, nnn, npn, e.north.uv, e.north.rotation, e.yRot % 180 == 0 ? 0 : e.xRot, e.uvLock, e.north.texture);
						}
					}
				}
			}
		}
	}
	return verts;
}

//TODO: multithreading? lol
unordered_map<pair<int32_t, int32_t>, vector<Vert>> verticizeChunks(const unordered_map<pair<int32_t, int32_t>, CompoundTag*>& worldNBT, Asset& ass)
{
	unordered_map<pair<int32_t, int32_t>, vector<Vert>> toReturn;

	for (const auto& [coords, chunkNBT] : worldNBT)
	{
		printf("vertisizing %i, %i\n", coords.first, coords.second);
		Chunk* chk = createChunk(chunkNBT, ass, coords);
		cullChunk(chk);
		toReturn[coords] = verticizeChunk(chk);
		toReturn[coords].shrink_to_fit();
	}
	return toReturn;
}