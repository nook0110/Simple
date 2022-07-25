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

//extern key_t psq_keys[12][64]; // filled randomly
//extern key_t enpass[8];        // during initialization
//extern key_t sideToMoveKey;

struct Position {

	std::array<Piece, 64> board;
	unsigned char sideToMove;
	square enPassantSquare = -1;

	key_t hash = 0;

	inline size_t address() const;

	std::array<bitboard, COLOR_NONE> color;
	std::array<bitboard, 12> pieces;

	// tapered evaluation data

	std::array<value, COLOR_NONE> nonPawnMaterial = { 0, 0 };
	std::array<value, PHASE_NONE> pieceValuesSum = { 0, 0 };
	std::array<value, PHASE_NONE> psqtBonusSum = { 0, 0 };

	Piece& operator[](const size_t ind) { return board[ind]; }
	const Piece& operator[](const size_t ind) const { return board[ind]; }

	std::array<square, COLOR_NONE> kingPos;

	// helpers

	void place(const Square& square, const Piece piece);
	void remove(const Square& square);

	void doMove(const Move& move);
	void undoMove(const Move& move);

	const bitboard pawns(const Color color) const;
	const bitboard queen(const Color color) const;
	const bitboard king(const Color color) const;
	bitboard pawnAttacks(const Color color) const;

	const bool underCheck(const Color us) const;

	// evaluation

	std::array<bitboard, COLOR_NONE> avaliableArea;
	std::array<value, PHASE_NONE> eval;

	const value evaluate();

	void init(std::string FEN, std::string move);


	std::vector<Move> generateAttacks();
	std::vector<Move> generateMoves();

	Move findBestMove();

	std::optional<value> findAlphaBeta(int depth, value alpha, value beta, const Move& previous);
	std::optional<value> quiesce(int depth, value alpha, value beta);
};

void findMove(Position& pos, const std::vector<Move>& moves, value& alpha, value& beta, Move& bestMove);

extern Position position;