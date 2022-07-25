#include "position.h"
#include "attacks.h"
#include <intrin.h>

#pragma intrinsic(_BitScanForward)

std::vector<Move> Position::generateAttacks()
{
	std::vector<Move> moves;
	moves.reserve(256);

	unsigned long sq;
	auto _pawns = pawns(static_cast<Color>(sideToMove));
	auto pieces = color[sideToMove] ^ _pawns;

	while (_BitScanForward64(&sq, pieces))
	{
		auto piece = board[sq];
		pieces &= ~SQUAREBB(sq);

		unsigned long dest;

		auto attacks = attack_map(static_cast<PieceType>(piece - shift[sideToMove]), sq, color[sideToMove] | color[!sideToMove]) & color[!sideToMove];

		while (_BitScanForward64(&dest, attacks))
		{
			auto consumable = board[dest];
			attacks &= ~SQUAREBB(dest);
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, consumable }));
		}

	}
	while (_BitScanForward64(&sq, _pawns))
	{
		auto pawn = board[sq];
		_pawns &= ~SQUAREBB(sq);

		unsigned long dest;
		auto column = sq & 7;
		auto row = sq >> 3;

		if (pawn == 'p')
		{
			if (row == 6)
			{
				if (column > 0)
				{
					dest = sq + 7;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column < 7)
				{
					dest = sq + 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				continue;
			}


			if (column > 0)
			{
				dest = sq + 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}
			if (column < 7)
			{
				dest = sq + 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}
			continue;
		}
		else
		{
			if (row == 1)
			{
				if (column < 7)
				{
					dest = sq - 7;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column > 0)
				{
					dest = sq - 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}

				continue;
			}


			if (column < 7)
			{
				dest = sq - 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}
			if (column > 0)
			{
				dest = sq - 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}

			continue;
		}
	}
	return moves;
}


std::vector<Move> Position::generateMoves()
{
	std::vector<Move> moves;
	moves.reserve(256);

	unsigned long sq;
	auto _pawns = pawns(static_cast<Color>(sideToMove));
	auto pieces = color[sideToMove] ^ _pawns;

	while (_BitScanForward64(&sq, pieces))
	{
		auto piece = board[sq];
		pieces &= ~SQUAREBB(sq);

		unsigned long dest;

		auto attacks = attack_map(static_cast<PieceType>(piece - shift[sideToMove]), sq, color[sideToMove] | color[!sideToMove]);

		while (_BitScanForward64(&dest, attacks))
		{
			auto consumable = board[dest];
			attacks &= ~SQUAREBB(dest);
			if (colorOf(consumable) != sideToMove)
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, consumable }));
		}

	}
	while (_BitScanForward64(&sq, _pawns))
	{
		auto pawn = board[sq];
		_pawns &= ~SQUAREBB(sq);

		unsigned long dest;
		auto column = sq & 7;
		auto row = sq >> 3;

		if (pawn == PAWN_B)
		{
			if (row == 6)
			{
				if (column > 0)
				{
					dest = sq + 7;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column < 7)
				{
					dest = sq + 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}

				dest = sq + 8;
				if (board[dest] == EMPTY)
					moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, EMPTY }));
				continue;
			}


			if (column > 0)
			{
				dest = sq + 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}
			if (column < 7)
			{
				dest = sq + 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}

			dest = sq + 8;
			if (board[dest] != EMPTY)
				continue;
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));

			if (row == 1)
			{
				dest = sq + 16;
				if (board[dest] != EMPTY)
					continue;
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));
			}

			continue;
		}
		else
		{
			if (row == 1)
			{
				if (column < 7)
				{
					dest = sq - 7;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column > 0)
				{
					dest = sq - 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}

				dest = sq - 8;
				if (board[dest] == EMPTY)
					moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, EMPTY }));
				continue;
			}


			if (column < 7)
			{
				dest = sq - 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}
			if (column > 0)
			{
				dest = sq - 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
			}

			dest = sq - 8;
			if (board[dest] != EMPTY)
				continue;
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));

			if (row == 6)
			{
				dest = sq - 16;
				if (board[dest] != EMPTY)
					continue;
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));
			}
			continue;
		}
	}
	return moves;
}
