#include "Decompressor.hpp"
#include <iostream>
#include <fstream>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "gunzip.hpp"

using namespace std;
using namespace std::filesystem;

Decompressor::Decompressor()
{
}

Decompressor::~Decompressor()
{
}


//takes the save folder, and decompresses and separates it by chunks identified by their global x/z coordinates in the map
map<pair<int, int>, vector<unsigned char>> Decompressor::decompress(string saveFolder)
{
	map<pair<int, int>, vector<unsigned char>> toReturn = map<pair<int, int>, vector<unsigned char>>();
	directory_iterator textureDir(saveFolder);
	for (auto f : textureDir)
	{
		//printf("seeing region file %s\n", f.path().u8string().c_str());
		if (f.path().filename() == "r.0.0.mca")//TEMP
		{
			basic_ifstream<unsigned char> file;
			file.exceptions(ios::failbit | ios::badbit);

			file.open(f.path().u8string(), ios_base::in | ios_base::binary);

			//get the size of the file
			file.seekg(0, ios::end);
			unsigned int length = file.tellg();
			file.seekg(ios::beg);

			unsigned char* buffer = new unsigned char[length]; //TODO unallocate this memory 
			file.read(buffer, length);
			

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
					int status = Deflate(&buffer[sos], sectorLength, [&](unsigned char c) {chunkData->push_back(c); });



				}
			}

		}
	}
}