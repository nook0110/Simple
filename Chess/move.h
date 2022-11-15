#pragma once
#include "globals.h"

struct Square {
	Square() : rank(-1), file(-1) {};
	square rank, file;
	Square(square rank, square file) :rank(rank), file(file) {}
	Square(square ind) : rank(ind >> 3), file(ind & 7) {}
	inline const square getInd() const noexcept { return SQUARE(rank, file); }
	const bool operator!=(const Square& other) const
	{
		if (rank != other.rank)
			return false;
		if (file != other.file)
			return false;	
		return true;
	}
	const bool operator==(const Square& other) const
	{
		return !(*this != other);
	}
};

enum MoveType {
	DOUBLE,
	EN_PASSANT,
	DEFAULT,
	CASTLING_OO,
	CASTLING_OOO,
	PROMOTION,
};

struct Move {
	Square from;
	Square to;
	MoveType moveType;
	Piece captured = EMPTY;

	std::string toStr() const noexcept
	{
		std::string str = (char)(from.file + 'a') + std::to_string(8 - from.rank) + (char)(to.file + 'a') + std::to_string(8 - to.rank);
		if (moveType == PROMOTION)
		{
			if (to.rank == 0)
			{
				str += "Q";
			}
			else
			{
				str += "q";
			}
		}
		return str;
	}

	const bool operator< (const Move& other) const
	{
		if (moveType != other.moveType)
			return moveType > other.moveType;
		if (captured != other.captured)
			if (captured != EMPTY && other.captured != EMPTY)
			{
				return std::abs(pieceValues[captured][MG]) > std::abs(pieceValues[other.captured][MG]);
			}
			else
				return captured != EMPTY;
		return abs(7 - 2 * to.file) + abs(7 - 2 * to.rank) < abs(7 - 2 * other.to.file) + abs(7 - 2 * other.to.rank);
	}
	const bool operator!=(const Move& other) const
	{
		if (from != other.from)
			return false;
		if (to != other.to)
			return false;
		if (moveType != other.moveType)
			return false;
		if (captured != other.captured)
			return false;
		return true;
	}
	const bool operator==(const Move& other) const
	{
		return !(*this != other);
	}
};