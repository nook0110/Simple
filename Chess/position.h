#pragma once
#include "move.h"
#include <fstream>
#include <bitset>
#include <array>
#include <string>

inline bool getColor(char& piece)
{
	return piece <= 'Z';
}

struct Position {
	std::string board;
	unsigned char sideToMove;
	unsigned char enPassantTargetColumn;
	unsigned char enPassantTargetRow;

	std::array<Move, 256> moves;
	unsigned char movesSize = 0;

	unsigned phase = 0;

	std::array<std::bitset<64>, 64> attackMap;
	std::array<std::bitset<64>, 2> color;

	inline void initAttackMap();
	void logAttackMap();

	void init(std::string FEN);

	char& operator[](const size_t ind) { return board[ind]; }
	const char& operator[](const size_t ind) const { return board[ind]; }
	inline void doMove(const Move& move);
	inline void undoMove(const Move& move);
};

extern Position position;
