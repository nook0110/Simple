#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <array>
#include <bitset>
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

inline constexpr signed pieceValues[13][PHASE_NONE] = { {126, 208}, {781, 854}, {825, 915}, {1276, 1380}, {2538, 2682}, {(int)1e7, (int)1e7},
												        {-126, -208}, {-781, -854}, {-825, -915}, {-1276, -1380}, {-2538, -2682}, {(int) - 1e7, (int) - 1e7},
												        {0, 0} };

enum Color
{
	COLOR_W = 0,
	COLOR_B,
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

constexpr uint_fast64_t file_a = 9259542123273814144;

inline constexpr std::array<std::bitset<64>, 8> FILES = 
{
	file_a,
	file_a >> 1,
	file_a >> 2,
	file_a >> 3,
	file_a >> 4,
	file_a >> 5,
	file_a >> 6,
	file_a >> 7
};

constexpr uint_fast64_t rank_1 = (1 << 8) - 1;

inline constexpr std::array<std::bitset<64>, 8> RANKS =
{
	rank_1,
	rank_1 << (8 * 1),
	rank_1 << (8 * 2),
	rank_1 << (8 * 3),
	rank_1 << (8 * 4),
	rank_1 << (8 * 5),
	rank_1 << (8 * 6),
	rank_1 << (8 * 7)
};

#endif // !GLOBALS_H