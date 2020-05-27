/*#include "Decompressor.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include "gunzip.hpp"





using namespace std;
using namespace std::filesystem;

Decompressor::Decompressor()
{
}

Decompressor::~Decompressor()
{
}

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
map<pair<int, int>, CompoundTag*> Decompressor::decompress(string saveFolder)
{
	map<pair<int, int>, CompoundTag*> toReturn = map<pair<int, int>, CompoundTag*>();
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

				vector<unsigned char>* chunkData = new vector<unsigned char>;
				int status = Deflate(&buffer[sos], sectorLength, [&](unsigned char c) {chunkData->push_back(c); });//decompress
				size_t initZero = 0;
				CompoundTag* chunkNBT = parseNBT(chunkData, &initZero);//convert decompressed byte data to NBT format

				//get the x and z of the chunk
				CompoundTag* root = chunkNBT->toCT();
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
}*/