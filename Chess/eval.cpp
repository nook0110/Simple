#include "position.h"

const value Position::evaluate()
{
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
	}
	return 0;
}