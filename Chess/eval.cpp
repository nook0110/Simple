#include "position.h"

constexpr value mobilityBonus[4][32][PHASE_NONE] =
{
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

const value Position::evaluate()
{
	eval = {0, 0};
	for (auto phase : { MG, EG })
	{
		eval[phase] += pieceValuesSum[phase];
		eval[phase] += psqtBonusSum[phase];
	}
	for (auto us : { COLOR_W, COLOR_B })
	{
		const Color them = flip(us);
		const bitboard low = (us == COLOR_W ? RANKS[1] | RANKS[2] : RANKS[6] | RANKS[5]);
		avaliableArea[us] = ~(pawnAttacks(them) | queen(us) | king(us) | (pawns(us) & low));
		mobility[us] = {0, 0, 0, 0};
		for (unsigned sq = 0; sq < 64; ++sq)
		{
			if (!avaliableArea[us].test(sq))
				continue;
			const bitboard attackers = attackMap[sq] & (color[us] ^ pawns(us));
			for (auto piece : { KNIGHT, BISHOP, ROOK, QUEEN })
				mobility[us][piece - 1] += (attackers & pieces[shift[us] + piece]).count();
		}
	}
	for (auto phase : { MG, EG })
	{
		for (auto piece : { KNIGHT, BISHOP, ROOK, QUEEN })
		{
			eval[phase] += mobilityBonus[piece - 1][mobility[COLOR_W][piece - 1]][phase];
			eval[phase] -= mobilityBonus[piece - 1][mobility[COLOR_B][piece - 1]][phase];
		}
	}
	return 0;
}

std::pair<value, value> Position::findAlphaBeta(int depth, Move move)
{
	const int maxDepth = 20;
	if (depth > maxDepth && move.captured == '-')
	{

	}

	return std::pair<value, value>();
}
