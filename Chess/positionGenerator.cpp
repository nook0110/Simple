#include "position.h"
#include "attacks.h"
#include <intrin.h>

#pragma intrinsic(_BitScanForward)

std::vector<Move> Position::generateAttacks()
{
	auto us = static_cast<Color>(sideToMove);

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
			doMove(moves.back());
			if (underCheck(us))
			{
				undoMove(moves.back());
				moves.pop_back();
			}
			else
			{
				undoMove(moves.back());
			}
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
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}
				if (column < 7)
				{
					dest = sq + 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}
				continue;
			}


			if (column > 0)
			{
				dest = sq + 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
			}
			if (column < 7)
			{
				dest = sq + 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
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
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}
				if (column > 0)
				{
					dest = sq - 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}

				continue;
			}


			if (column < 7)
			{
				dest = sq - 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
			}
			if (column > 0)
			{
				dest = sq - 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
			}

			continue;
		}
	}
	return moves;
}


std::vector<Move> Position::generateMoves()
{
	auto us = static_cast<Color>(sideToMove);

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
			{
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, consumable }));
				doMove(moves.back());
				if (underCheck(us))
				{
					undoMove(moves.back());
					moves.pop_back();
				}
				else
				{
					undoMove(moves.back());
				}
			}
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
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}
				if (column < 7)
				{
					dest = sq + 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}

				dest = sq + 8;
				if (board[dest] == EMPTY)
				{
					moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				continue;
			}


			if (column > 0)
			{
				dest = sq + 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
			}
			if (column < 7)
			{
				dest = sq + 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
			}

			dest = sq + 8;
			if (board[dest] != EMPTY)
				continue;
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));
			doMove(moves.back());
			if (underCheck(us))
			{
				undoMove(moves.back());
				moves.pop_back();
			}
			else
			{
				undoMove(moves.back());
			}
			if (row == 1)
			{
				dest = sq + 16;
				if (board[dest] != EMPTY)
					continue;
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));
				doMove(moves.back());
				if (underCheck(us))
				{
					undoMove(moves.back());
					moves.pop_back();
				}
				else
				{
					undoMove(moves.back());
				}
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
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}
				if (column > 0)
				{
					dest = sq - 9;
					if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
					{
						moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, board[dest] }));
						doMove(moves.back());
						if (underCheck(us))
						{
							undoMove(moves.back());
							moves.pop_back();
						}
						else
						{
							undoMove(moves.back());
						}
					}
				}

				dest = sq - 8;
				if (board[dest] == EMPTY)
				{
					moves.push_back(Move({ Square(sq), Square(dest), PROMOTION, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				continue;
			}


			if (column < 7)
			{
				dest = sq - 7;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
			}
			if (column > 0)
			{
				dest = sq - 9;
				if (board[dest] != EMPTY && colorOf(board[dest]) != sideToMove)
				{
					moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, board[dest] }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
				if (dest == enPassantSquare)
				{
					moves.push_back(Move({ Square(sq), Square(dest), EN_PASSANT, EMPTY }));
					doMove(moves.back());
					if (underCheck(us))
					{
						undoMove(moves.back());
						moves.pop_back();
					}
					else
					{
						undoMove(moves.back());
					}
				}
			}

			dest = sq - 8;
			if (board[dest] != EMPTY)
				continue;
			moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));
			doMove(moves.back());
			if (underCheck(us))
			{
				undoMove(moves.back());
				moves.pop_back();
			}
			else
			{
				undoMove(moves.back());
			}
			if (row == 6)
			{
				dest = sq - 16;
				if (board[dest] != EMPTY)
					continue;
				moves.push_back(Move({ Square(sq), Square(dest), DEFAULT, EMPTY }));
				doMove(moves.back());
				if (underCheck(us))
				{
					undoMove(moves.back());
					moves.pop_back();
				}
				else
				{
					undoMove(moves.back());
				}
			}
			continue;
		}
	}
	return moves;
}
