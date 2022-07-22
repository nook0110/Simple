#ifndef SEARCH_H
#define SEARCH_H
#include "position.h"
#include <vector>

enum nodeType
{
	PV_NODE,
	CUT_NODE,
	ALL_NODE,
	NODE_TYPE_NONE
};

struct nodeInfo
{
	nodeInfo() : depth(-1) {}
	//nodeInfo() : type(NODE_TYPE_NONE) {}
	key_t key;
	unsigned char depth;
	//nodeType type;
	Move bestMove;
};

constexpr size_t tableSize = 1ull << 27 - 1ull;

inline size_t Position::address() const
{
	return hash & tableSize;
}

//extern nodeInfo* TTable = new nodeInfo[tableSize];

#endif // !SEARCH_H