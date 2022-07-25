#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <array>
#include <bitset>
#include <unordered_map>
#include <cmath>
#include <cassert>

#ifdef _MSC_VER
#pragma warning(disable : 26812)
#endif // _MSC_VER

using bitboard = uint_fast64_t;
using square = char;

#define SQUARE(x,y) ((x) << 3 | (y))
#define SQUAREBB(sq) (1ull << (sq))

constexpr bitboard EMPTY_BOARD = 0;

enum PieceType
{
	PAWN = 0,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	PT_NONE
};

enum Piece
{
	PAWN_W = 0,
	KNIGHT_W,
	BISHOP_W,
	ROOK_W,
	QUEEN_W,
	KING_W,
	PAWN_B,
	KNIGHT_B,
	BISHOP_B,
	ROOK_B,
	QUEEN_B,
	KING_B,
	EMPTY,
	PIECE_NONE
};

enum Color
{
	COLOR_B = 0,
	COLOR_W,
	COLOR_NONE
};

/*
enum square
{
	A8, B8, C8, D8, E8, F8, G8, H8,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A1, B1, C1, D1, E1, F1, G1, H1,
	SQ_NONE
};

inline square& operator++(square& sq) { return sq = (square)((int)sq + 1); }
inline square operator+=(square& sq, square& oth) { return sq = (square)(sq + (int)oth); }
*/

inline std::unordered_map<char, const Piece> PIECES = { {'P', PAWN_W}, {'N', KNIGHT_W}, {'B', BISHOP_W}, {'R', ROOK_W}, {'Q', QUEEN_W}, {'K', KING_W},
													  {'p', PAWN_B}, {'n', KNIGHT_B}, {'b', BISHOP_B}, {'r', ROOK_B}, {'q', QUEEN_B}, {'k', KING_B},
													  {'-', EMPTY} };

enum DiscretePhase
{
	MG,
	EG,
	PHASE_NONE
};

inline constexpr signed pieceValues[13][PHASE_NONE] = { {126, 208}, {781, 854}, {825, 915}, {1276, 1380}, {2538, 2682}, {0, 0},
												        {-126, -208}, {-781, -854}, {-825, -915}, {-1276, -1380}, {-2538, -2682}, {0, 0},
												        {0, 0} };

inline constexpr std::array<const int, COLOR_NONE> shift = { 6, 0 };
	
inline Color flip(const Color c)
{
	if (c == COLOR_NONE)
		return COLOR_NONE;
	return static_cast<Color>(!c);
}

inline Color colorOf(const Piece piece)
{
	if (piece == EMPTY || piece == PIECE_NONE)
		return COLOR_NONE;
	return (piece <= KING_W ? COLOR_W : COLOR_B);
}

inline const bool nonPawn(const Piece piece)
{
    return !(piece == PAWN_W || piece == PAWN_B);
}

constexpr uint_fast64_t h = 9259542123273814144;

inline constexpr std::array<bitboard, 8> FILES = 
{
	h >> 7,
	h >> 6,
	h >> 5,
	h >> 4,
	h >> 3,
	h >> 2,
	h >> 1,
	h
};

constexpr uint_fast64_t highestRank = (1 << 8) - 1;

inline constexpr std::array<bitboard, 8> RANKS =
{
	highestRank << (8 * 7),
	highestRank << (8 * 6),
	highestRank << (8 * 5),
	highestRank << (8 * 4),
	highestRank << (8 * 3),
	highestRank << (8 * 2),
	highestRank << (8 * 1),
	highestRank
};


extern unsigned long long counter;

#endif // !GLOBALS_H