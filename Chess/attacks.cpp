#include "attacks.h"

unsigned bishopBase[64];
unsigned rookBase[64];

bitboard bishop_attacks[6000];
bitboard rook_attacks[105000];

bitboard bishop_attack(const square sq, const bitboard occ)
{
	bitboard map = EMPTY_BOARD;
	bitboard free = ~occ;
	for (int step : {-9, -7, 7, 9})
	{
		square s = sq;
		while (free & (1ull << s))
		{
			square to = s + step;
			if (to < 0 || to > 63)
				break;
			int dist = (s & 7) - (to & 7);
			if (dist < -1 || dist > 1)
				break;
			map |= (1ull << to);
			s = to;
		}
	}
	return map;
}

bitboard rook_attack(const square sq, const bitboard occ)
{
	bitboard map = EMPTY_BOARD;
	bitboard free = ~occ;
	for (int step : {-8, -1, 1, 8})
	{
		square s = sq;
		while (free & (1ull << s))
		{
			square to = s + step;
			if (to < 0 || to > 63)
				break;
			int dist = (s & 7) - (to & 7);
			if (dist < -1 || dist > 1)
				break;
			map |= (1ull << to);
			s = to;
		}
	}
	return map;
}

void init_sliding_maps()
{
	unsigned long long size = 0;
	bitboard submask = EMPTY_BOARD;
	for (square sq = 0; sq < 64; ++sq)
	{
		bishopBase[sq] = (sq == 0 ? 0 : bishopBase[sq - 1] + size);
		submask = EMPTY_BOARD;
		bitboard mask = bishopMask[sq];
		size = 0;
		do
		{
			bishop_attacks[bishopBase[sq] + _pext_u64(submask, mask)] = bishop_attack(sq, submask);
			submask = (submask - mask) & mask;
			++size;
		} while (submask);
	}
	size = 0;
	submask = EMPTY_BOARD;
	for (square sq = 0; sq < 64; ++sq)
	{
		rookBase[sq] = (sq == 0 ? 0 : rookBase[sq - 1] + size);
		submask = EMPTY_BOARD;
		bitboard mask = rookMask[sq];
		size = 0;
		do
		{
			rook_attacks[rookBase[sq] + _pext_u64(submask, mask)] = rook_attack(sq, submask);
			submask = (submask - mask) & mask;
			++size;
		} while (submask);
	}
}

std::string prettyBB(bitboard b, bitboard occ)
{

	std::string s = "+---+---+---+---+---+---+---+---+\n";

	for (int r = 0; r < 8; ++r)
	{
		for (int f = 0; f < 8; ++f)
		{
			s += "| ";
			bool at = b & (1ull << SQUARE(r, f));
			bool oc = occ & (1ull << SQUARE(r, f));
			if (at)
				s += "X ";
			else if (oc)
				s += "O ";
			else
				s += "  ";
		}

		s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
	}
	s += "  a   b   c   d   e   f   g   h\n";

	return s;
}
