#pragma once
#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "Asset.hpp"
using namespace std;

struct Coord
{
	int64_t x;
	int64_t y;
	int64_t z;
	Coord operator+(const Coord& other) const
	{
		return Coord{ other.x + x, other.y + y, other.z + z };
	}
	Coord operator-(const Coord& other) const
	{
		return Coord{ x-other.x, y-other.y, z-other.z };
	}
};



struct Block
{
	string blockstate;
	vector<string_view> attributes;
};

//struct BlockID
//{
//	uint16_t id : 12;
//	uint16_t meta : 4;
//};
//struct Block
//{
//	BlockID data;
//	uint8_t faces; //this is used for true false on which faces should be rendered going null/null/+y/-y/+x/-x/+z/-z
//};
struct Section
{
	int y;
	vector<Model> palette;
	array<array<array<Model, 16>, 16>, 16> blocks;
};

namespace std
{
	template<>
	struct hash<pair<int32_t, int32_t>>
	{
		size_t operator ()(const pair<int32_t, int32_t>& value) const
		{
			uint64_t key = ((uint64_t)value.first) << 32 | (uint64_t)value.second;
			key ^= (key >> 33);
			key *= 0xff51afd7ed558ccd;
			key ^= (key >> 33);
			key *= 0xc4ceb9fe1a85ec53;
			key ^= (key >> 33);
			return (size_t)key;
		}
	};
}

struct Chunk
{
	int x;
	int z;
	unordered_map<uint32_t, Section*> sections;
	array<array<array<uint32_t, 64>, 4>, 4> biomes;
};
struct Region
{
	int x;
	int z;
	unordered_map<pair<int32_t, int32_t>, Chunk*> chunks;
};
struct World
{
	unordered_map<pair<int32_t, int32_t>, Region*> regions;
};

#endif