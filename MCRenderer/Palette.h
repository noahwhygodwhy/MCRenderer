#include <vector>
#include <cstdint>
#include "Chunk.h"

using namespace std;

class Palette
{
	vector<Block> values;
public:
	Palette();
	~Palette();
	Block getValue(string_view name); //idk if this should be strinview
private:
};
