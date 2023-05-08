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
	nodeInfo() : depth(255), maxDepth(255) {}
	nodeInfo(unsigned char depth, value eval, unsigned char maxDepth) : depth(depth), eval(eval), maxDepth(maxDepth) {}
	//nodeInfo() : type(NODE_TYPE_NONE) {}
	//key_t key;
	unsigned char depth;
	unsigned char maxDepth;
	value eval;
	//nodeType type;
	Move bestMove;
};

constexpr size_t tableSize = (1ull << 27) - 1ull;

inline size_t Position::address() const
{
	return hash & tableSize;
}

#endif // !SEARCH_H