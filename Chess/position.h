#pragma once
#include "move.h"
#include "eval.h"
#include <fstream>
#include <bitset>
#include <array>
#include <string>
#include "positionGenerator.h"
#include <omp.h>
#include <optional>
#include <mutex>

using key_t = uint_fast64_t;

// Zobrist hash keys

key_t psq_keys[12][64]; // filled randomly
key_t enpass[8];        // during initialization
key_t sideToMoveKey;

struct Position {
	std::string board = "----------------------------------------------------------------";
	unsigned char sideToMove;
	square enPassantSquare=-1;

	key_t hash = 0;

	inline size_t address() const;

	std::array<std::bitset<64>, 64> attackMap;
	std::array<std::bitset<64>, COLOR_NONE> color;
	std::array<std::bitset<64>, 12> pieces;

	// tapered evaluation data

	std::array<value, COLOR_NONE> nonPawnMaterial = { 0, 0 };
	std::array<value, PHASE_NONE> pieceValuesSum = { 0, 0 };
	std::array<value, PHASE_NONE> psqtBonusSum = { 0, 0 };

	char& operator[](const size_t ind) { return board[ind]; }
	const char& operator[](const size_t ind) const { return board[ind]; }

	std::array<square, 2> kingPos;

	// helpers

	void place(const Square& square, const char piece);
	void remove(const Square& square);

	void doMove(const Move& move);
	void undoMove(const Move& move);

	void updatePiece(const Square& square);
	void updateEmptySquare(const Square& square);
	void updateOccupiedSquare(const Square& square);

	const std::bitset<64>& pawns(const Color color) const;
	const std::bitset<64>& queen(const Color color) const;
	const std::bitset<64>& king(const Color color) const;
	std::bitset<64> pawnAttacks(const Color color) const;

	const bool underCheck(const Color us) const;

	// evaluation

	std::array<std::bitset<64>, COLOR_NONE> avaliableArea;
	std::array<std::array<unsigned, 5>, COLOR_NONE> mobility;
	std::array<value, PHASE_NONE> eval;

	const value evaluate();

	void init(std::string FEN, std::string move);



	std::vector<Move> generateMoves();

	Move findBestMove();

	std::optional<value> findAlphaBeta(int depth, value alpha, value beta , const Move& previous);
};

void findMove(Position& pos, const std::vector<Move>& moves, value& alpha, value& beta, Move& bestMove);

extern Position position;