#include "position.h"
#include "attacks.h"
#include <intrin.h>

#pragma intrinsic(_BitScanForward)

std::vector<Move> Position::generateAttacks()
{
	std::vector<Move> moves;
	moves.reserve(256);

	unsigned long sq;
	auto pieces = color[sideToMove] & ~pawns(static_cast<Color>(sideToMove));
	auto _pawns = pawns(static_cast<Color>(sideToMove));
	while (_BitScanForward64(&sq, pieces))
	{
		char piece;
		piece = board[sq];
		pieces &= ~SQUAREBB(sq);

		unsigned long dest;

		auto attacks = attack_map(static_cast<Piece>(PIECES[piece] - shift[sideToMove]), sq, color[sideToMove] | color[!sideToMove]) & color[!sideToMove];

		while (_BitScanForward64(&dest, attacks))
		{
			auto consumable = board[dest];
			attacks &= ~SQUAREBB(dest);
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, consumable }));
		}

	}
	while (_BitScanForward64(&sq, _pawns))
	{
		char pawn = board[sq];
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
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column < 7)
				{
					dest = sq + 9;
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				continue;
			}


			if (column > 0)
			{
				dest = sq + 7;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
			}
			if (column < 7)
			{
				dest = sq + 9;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
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
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column > 0)
				{
					dest = sq - 9;
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}

				continue;
			}


			if (column < 7)
			{
				dest = sq - 7;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
			}
			if (column > 0)
			{
				dest = sq - 9;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
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
	auto pieces = color[sideToMove] & ~pawns(static_cast<Color>(sideToMove));
	auto _pawns = pawns(static_cast<Color>(sideToMove));
	while (_BitScanForward64(&sq, pieces))
	{
		char piece = board[sq];
		pieces &= ~SQUAREBB(sq);

		unsigned long dest;

		auto attacks = attack_map(static_cast<Piece>(PIECES[piece] - shift[sideToMove]), sq, color[sideToMove] | color[!sideToMove]);

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
		char pawn = board[sq];
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
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column < 7)
				{
					dest = sq + 9;
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}

				dest = sq + 8;
				if (board[dest] == '-')
					moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, '-' }));
				continue;
			}


			if (column > 0)
			{
				dest = sq + 7;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
			}
			if (column < 7)
			{
				dest = sq + 9;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
			}

			dest = sq + 8;
			if (board[dest] != '-')
				continue;
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, '-' }));

			if (row == 1)
			{
				dest = sq + 16;
				if (board[dest] != '-')
					continue;
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, '-' }));
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
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}
				if (column > 0)
				{
					dest = sq - 9;
					if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
				}

				dest = sq - 8;
				if (board[dest] == '-')
					moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, '-' }));
				continue;
			}


			if (column < 7)
			{
				dest = sq - 7;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
			}
			if (column > 0)
			{
				dest = sq - 9;
				if (board[dest] != '-' && colorOf(board[dest]) != sideToMove)
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
				if (dest == enPassantSquare)
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, '-' }));
			}

			dest = sq - 8;
			if (board[dest] != '-')
				continue;
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, '-' }));

			if (row == 6)
			{
				dest = sq - 16;
				if (board[dest] != '-')
					continue;
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, '-' }));
			}
			continue;
		}
	}
	return moves;
}
