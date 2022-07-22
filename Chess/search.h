#ifndef SEARCH_H
#define SEARCH_H
#include "position.h"
#include <vector>

enum nodeType
{
	PV_NODE,
	ALL_NODE,
	CUT_NODE,
	NODE_TYPE_NONE
};

struct nodeInfo
{
	nodeInfo() : type(NODE_TYPE_NONE) {}
	key_t key;
	short int depth;
	nodeType type;
	Move bestMove;
};

constexpr size_t tableSize = 1ull << 32 - 1ull;

inline size_t Position::address() const
{
	return hash & tableSize;
}

extern std::vector<nodeInfo> tp_table(tableSize, nodeInfo());

#endif // !SEARCH_H