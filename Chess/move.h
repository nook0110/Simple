#pragma once
#include "globals.h"

struct Square {
	Square() {};
	square rank, file;
	Square(square rank, square file) :rank(rank), file(file) {}
	Square(square ind) : rank(ind >> 3), file(ind & 7) {}
	inline const square getInd() const noexcept { return SQUARE(rank, file); }
};

enum MoveType {
	DOUBLE,
	EN_PASSANT,
	DEFAULT,
	PROMOTION,
};

struct Move {
	Square from;
	Square to;
	MoveType moveType;
	char captured = '-';

	std::string toStr() const noexcept
	{
		std::string str = (char)(from.file + 'a') + std::to_string(8 - from.rank) + " " + (char)(to.file + 'a') + std::to_string(8 - to.rank);
		if (moveType == PROMOTION)
		{
			if (to.rank == 0)
			{
				str += " Q";
			}
			else
			{
				str += " q";
			}
		}
		return str;
	}

	const bool operator< (const Move& other) const
	{
		if (moveType != other.moveType)
			return moveType > other.moveType;
		if (captured != other.captured)
			if (captured != '-' && other.captured != '-')
				return PIECES[captured] > PIECES[other.captured];
			else
				return PIECES[captured] < PIECES[other.captured];
		return abs(7 - 2 * to.file) + abs(7 - 2 * to.rank) < abs(7 - 2 * other.to.file) + abs(7 - 2 * other.to.rank);
	}
};