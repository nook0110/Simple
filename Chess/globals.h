#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <array>
#include <unordered_map>

using square = char;

#define SQUARE(x,y) (x << 3 | y)

inline std::unordered_map<char, const int> PIECES = { {'P', 0}, {'N', 1}, {'B', 2}, {'R', 3}, {'Q', 4}, {'K', 5},
													  {'p', 6}, {'n', 7}, {'b', 8}, {'r', 9}, {'q', 10}, {'k', 11},
													  {'-', 12} };

enum DiscretePhase
{
	MG,
	EG,
	PHASE_NONE
};

constexpr signed pieceValues[13][PHASE_NONE] = { {126, 208}, {781, 854}, {825, 915}, {1276, 1380}, {2538, 2682}, {1e7, 1e7},
												{-126, -208}, {-781, -854}, {-825, -915}, {-1276, -1380}, {-2538, -2682}, {-1e7, -1e7},
												{0, 0} };

enum Color
{
	COLOR_W = 0,
	COLOR_B,
	COLOR_EMPTY,
    COLOR_NONE
};

inline Color colorOf(const char piece)
{
	if (PIECES[piece] == 12)
		return COLOR_NONE;
	return (piece <= 'Z' ? COLOR_W : COLOR_B);
}

inline const bool nonPawn(const char piece)
{
    return !(piece == 'P' || piece == 'p');
}

#endif // !GLOBALS_H