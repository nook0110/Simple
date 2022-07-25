#include "position.h"
#include "attacks.h"
#include <iostream>
#include <intrin.h>
#include <nmmintrin.h>

constexpr value mobilityBonus[5][32][PHASE_NONE] =
{
	{}, // P
	{
		{-62, -79}, {-53, -57}, {-12, -31}, {-3, -17},
		{3, 7}, {12, 13}, {21, 16}, {28, 21}, {37, 26} // N
	},
	{
		{-47, -59}, {-20, -25}, {14, -8}, {29, 12}, {39, 21}, {53, 40}, {53, 56},
		{60, 58}, {62, 65}, {69, 72}, {78, 78}, {83, 87}, {91, 88}, {96, 98} // B
	},
	{
		{-60, -82}, {-24, -15}, {0, 17}, {3, 43}, {4, 72}, {14, 100}, {20, 102}, {30, 122},
		{41, 133}, {41, 139}, {41, 153}, {45, 160}, {57, 165}, {58, 170}, {67, 175} // R
	},
	{
		{-29, -49}, {-16, -29}, {-8, -8}, {-8, 17}, {18, 39}, {25, 54}, {23, 59}, {37, 73},
		{41, 76}, {54, 95}, {65, 95}, {68, 101}, {69, 124}, {70, 128}, {70, 132},
		{70, 133}, {71, 136}, {72, 140}, {74, 147}, {76, 149}, {90, 153}, {104, 169},
		{105, 171}, {106, 171}, {112, 178}, {114, 185}, {114, 187}, {119, 121} // Q
	}
};

value mob[COLOR_NONE][PHASE_NONE];

constexpr std::array<value, PHASE_NONE> limits = { 15258, 3915 };
constexpr value rangeLength = limits[MG] - limits[EG];
constexpr value tempoBonus = 28;

const value Position::evaluate()
{
	eval = { 0, 0 };

	for (auto phase : { MG, EG })
	{
		eval[phase] += pieceValuesSum[phase];
		eval[phase] += psqtBonusSum[phase];
	}

	const bitboard all = color[COLOR_W] | color[COLOR_B];

	for (const auto us : { COLOR_W, COLOR_B })
	{
		const Color them = flip(us);
		const bitboard low = (us == COLOR_W ? RANKS[1] | RANKS[2] : RANKS[6] | RANKS[5]);
		const bitboard blocked = (us == COLOR_W ? all << 8 : all >> 8) & pawns(us);
		avaliableArea[us] = ~(pawnAttacks(them) | queen(us) | king(us) | (pawns(us) & low) | blocked);
		mob[us][MG] = 0;
		mob[us][EG] = 0;
		for (auto piece : { KNIGHT, BISHOP, ROOK, QUEEN })
		{
			bitboard units = pieces[piece + shift[us]];
			while (units)
			{
				unsigned long sq = 0;
				_BitScanForward64(&sq, units);
				units &= (units - 1);
				bitboard xray = (piece == ROOK ? pieces[ROOK + shift[us]] | queen(us) | queen(them) :
					(piece == BISHOP ? queen(us) | queen(them) : EMPTY_BOARD));
				bitboard attacks = attack_map(piece, sq, all ^ xray);
				unsigned mobility = _mm_popcnt_u64(attacks & avaliableArea[us]);
				mob[us][MG] += mobilityBonus[piece][mobility][MG];
				mob[us][EG] += mobilityBonus[piece][mobility][EG];
			}
		}
	}

	eval[MG] += (mob[COLOR_W][MG] - mob[COLOR_B][MG]);
	eval[EG] += (mob[COLOR_W][EG] - mob[COLOR_B][EG]);

	value npm = nonPawnMaterial[COLOR_W] - nonPawnMaterial[COLOR_B];
	npm = std::max(limits[EG], std::min(limits[MG], npm));
	value phase = npm - limits[EG];
	value tapered = (eval[MG] * phase + eval[EG] * (rangeLength - phase));
	value tempo = tempoBonus * rangeLength * (sideToMove ? 1 : -1);
	return tapered + tempo;
}