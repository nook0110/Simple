#pragma once
#include "move.h"
#include "eval.h"
#include <fstream>
#include <bitset>
#include <array>
#include <string>
#include "positionGenerator.h"
#include <omp.h>

inline square findSquare(const std::bitset<64>& bitset) // function that finds '1' in bitset
{
	for (square i = 0; i < bitset.size(); ++i)
	{
		if (bitset.test(i))
			return i;
	}
	return 64;
}

struct Position {
	std::string board = "----------------------------------------------------------------";
	unsigned char sideToMove;
	square enPassantSquare;

	std::array<std::bitset<64>, 64> attackMap;
	std::array<std::bitset<64>, COLOR_NONE> color;
	std::array<std::bitset<64>, 12> pieces;

	// tapered evaluation data

	std::array<value, COLOR_NONE> nonPawnMaterial = { 0, 0 };
	std::array<value, PHASE_NONE> pieceValuesSum = { 0, 0 };
	std::array<value, PHASE_NONE> psqtBonusSum = { 0, 0 };

	char& operator[](const size_t ind) { return board[ind]; }
	const char& operator[](const size_t ind) const { return board[ind]; }

	// helpers

	void place(const Square& square, const char piece);
	void remove(const Square& square);

	void doMove(const Move& move);
	void undoMove(const Move& move);

	void updatePiece(const Square& square);
	void updateEmptySquare(const Square& square);
	void updateOccupiedSquare(const Square& square);

	std::bitset<64>& pawns(const Color color);
	std::bitset<64>& queen(const Color color);
	std::bitset<64>& king(const Color color);
	std::bitset<64> pawnAttacks(const Color color);

	// evaluation

	std::array<std::bitset<64>, COLOR_NONE> avaliableArea;
	std::array<std::array<unsigned, 5>, COLOR_NONE> mobility;
	std::array<value, PHASE_NONE> eval;

	const value evaluate();

	void init(std::string FEN, std::string move);
	void read();
	void log();

	void readAttackMap();
	void logAttackMap();

	void readColor();
	void logColor();

	void readPieces();
	void logPieces();

	std::vector<Move> generateMoves();

	Move findBestMove();

	value findAlphaBeta(int depth = 1, value alpha = INT_MIN, value beta = INT_MAX,const Move& previous = Move());
};

extern Position position;