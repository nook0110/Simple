#include "position.h"
#include "pieceSquareTables.hpp"

void Position::updatePiece(const Square& sq)
{
	square piece = sq.getInd();
	square dest = piece;
	square delta;
	switch (board[dest])
	{
	case 'r':
	case 'R':
		delta = 1;
		dest += delta;
		while (dest % 8 != 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -1;
		dest = piece + delta;
		while (dest % 8 != 7 && dest > 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = 8;
		dest = piece + delta;
		while (dest < 64)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -8;
		dest = piece + delta;
		while (dest >= 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		break;
	case 'B':
	case 'b':
		/*
		|-9 -8 -7|
		|-1  0  1|
		|7   8  9|
		*/
		delta = -7;
		dest = piece + delta;
		while (dest % 8 != 0 && dest > 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -9;
		dest = piece + delta;
		while (dest % 8 != 7 && dest > 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = 9;
		dest = piece + delta;
		while (dest % 8 != 0 && dest < 64)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = 7;
		dest = piece + delta;
		while (dest % 8 != 7 && dest < 64)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		break;
	case 'q':
	case 'Q':
		delta = 1;
		dest += delta;
		while (dest % 8 != 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -1;
		dest = piece + delta;
		while (dest % 8 != 7 && dest > 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = 8;
		dest = piece + delta;
		while (dest < 64)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -8;
		dest = piece + delta;
		while (dest >= 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		/*
		|-9 -8 -7|
		|-1  0  1|
		|7   8  9|
		*/
		/*
		|-9 -8 -7|
		|-1  0  1|
		|7   8  9|
		*/
		delta = -7;
		dest = piece + delta;
		while (dest % 8 != 0 && dest > 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -9;
		dest = piece + delta;
		while (dest % 8 != 7 && dest > 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = 9;
		dest = piece + delta;
		while (dest % 8 != 0 && dest < 64)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = 7;
		dest = piece + delta;
		while (dest % 8 != 7 && dest < 64)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		break;
	case 'k':
	case 'K':
		/*
		|-9 -8 -7|
		|-1  0  1|
		|7   8  9|
		*/
	{
		constexpr std::array<square, 8> deltas = { -9,-8,-7,-1,1,7,8,9 };
		for (auto d : deltas)
		{
			square current = piece + d;
			if (current >= 0 && current < 64 && (abs((current >> 3) - (piece >> 3)) <= 1 && abs((current & 7) - (piece & 7)) <= 1))
				attackMap[current].set(piece, 1);
		}
	}
	break;
	case 'n':
	case 'N':
		/*
		|-18 -17 -16 -15 -14|
		|-10  -9  -8  -7  -6|
		| -2  -1   0   1   2|
		|  6   7   8   9  10|
		| 14  15  16  17  18|
		*/

	{
		constexpr std::array<square, 8> deltas = { -17,-15,-10,-6,6,10,15,17 };
		for (auto d : deltas)
		{
			dest = piece + d;
			if (dest >= 0 && dest < 64 && (abs((dest >> 3) - (piece >> 3)) <= 2 && abs((dest & 7) - (piece & 7)) <= 2))
				attackMap[dest].set(piece, 1);
		}
	}
	break;
	case 'p':
		if (piece >> 3 == 1)
		{
			attackMap[SQUARE(3, piece & 7)].set(piece, 1);
		}
		attackMap[piece + 8].set(piece, 1);
		break;
	case 'P':
		if (piece >> 3 == 6)
		{
			attackMap[SQUARE(4, piece & 7)].set(piece, 1);
		}
		attackMap[piece - 8].set(piece, 1);
		break;
	case '-':
		for (auto& attack : attackMap)
		{
			attack.set(piece, 0);
		}
		break;
	default:
		break;
	}
}

void  Position::updateEmptySquare(const Square& sq)
{
	square dest = sq.getInd();
	auto attackers = attackMap[dest];
	attackers.set(dest, 0);
	square piece;
	while (attackers.any())
	{
		piece = findSquare(attackers);
		attackers.set(piece, 0);
		square delta = dest - piece;
		switch (board[piece])
		{
		case 'r':
		case 'R':
			if (delta % (SQUARE(1, 0) - SQUARE(0, 0)) != 0)
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest % 8 != 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest += 1;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest > 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest -= 1;
					}
				}
			}
			else
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest < 64)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest += 8;
					}
				}
				else
				{
					while (dest >= 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest -= 8;
					}
				}
			}
			break;
		case 'B':
		case 'b':
			/*
			|-9 -8 -7|
			|-1  0  1|
			|7   8  9|
			*/
			if (delta < 0)
			{
				if (delta % 7 == 0)
				{
					while (dest % 8 != 0 && dest > 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest -= 7;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest > 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest -= 9;
					}
				}
			}
			else
			{
				if (delta % 9 == 0)
				{
					while (dest % 8 != 0 && dest < 64)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest += 9;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest < 64)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest += 7;
					}
				}
			}
		case 'q':
		case 'Q':
			if (delta % 8 == 0)
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest < 64)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest += 8;
					}
				}
				else
				{
					while (dest >= 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest -= 8;
					}
				}
			}
			else if (dest >> 3 == piece >> 3)
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest % 8 != 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest += 1;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest > 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest -= 1;
					}
				}
			}
			else
			{
				if (delta < 0)
				{
					if (delta % 7 == 0)
					{
						while (dest % 8 != 0 && dest > 0)
						{
							attackMap[dest].set(piece, 1);
							if (board[dest] != '-')
								break;
							dest -= 7;
						}
					}
					else
					{
						while (dest % 8 != 7 && dest > 0)
						{
							attackMap[dest].set(piece, 1);
							if (board[dest] != '-')
								break;
							dest -= 9;
						}
					}
				}
				else
				{
					if (delta % 9 == 0)
					{
						while (dest % 8 != 0 && dest < 64)
						{
							attackMap[dest].set(piece, 1);
							if (board[dest] != '-')
								break;
							dest += 9;
						}
					}
					else
					{
						while (dest % 8 != 7 && dest < 64)
						{
							attackMap[dest].set(piece, 1);
							if (board[dest] != '-')
								break;
							dest += 7;
						}
					}
				}
			}
		default:
			break;
		}
	}
}

void  Position::updateOccupiedSquare(const Square& sq)
{
	square dest = sq.getInd();
	auto attackers = attackMap[dest];
	square piece;
	while (attackers.any())
	{
		piece = findSquare(attackers);
		attackers.set(piece, 0);
		square delta = dest - piece;
		switch (board[piece])
		{
		case 'r':
		case 'R':
			if (delta % 8 != 0)
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest % 8 != 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest += 1;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest > 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest -= 1;
					}
				}
			}
			else
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest < 64)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest += 8;
					}
				}
				else
				{
					while (dest >= 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest -= 8;
					}
				}
			}
			break;
		case 'B':
		case 'b':
			if (delta < 0)
			{
				if (delta % 7 == 0)
				{
					while (dest % 8 != 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest -= 7;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest > 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest -= 9;
					}
				}
			}
			else
			{
				if (delta % 9 == 0)
				{
					while (dest % 8 != 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest += 9;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest > 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest += 7;
					}
				}
			}
		case 'q':
		case 'Q':
			if (delta % 8 == 0)
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest < 64)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest += 8;
					}
				}
				else
				{
					while (dest >= 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest -= 8;
					}
				}
			}
			else if (dest >> 3 == piece >> 3)
			{
				square sign = (delta > 0) - (delta < 0);
				if (sign > 0)
				{
					while (dest % 8 != 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest += 1;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest > 0)
					{
						attackMap[dest].set(piece, 0);
						if (board[dest] != '-')
							break;
						dest -= 1;
					}
				}
			}
			else
			{
				if (delta < 0)
				{
					if (delta % 7 == 0)
					{
						while (dest % 8 != 0)
						{
							attackMap[dest].set(piece, 0);
							if (board[dest] != '-')
								break;
							dest -= 7;
						}
					}
					else
					{
						while (dest % 8 != 7 && dest > 0)
						{
							attackMap[dest].set(piece, 0);
							if (board[dest] != '-')
								break;
							dest -= 9;
						}
					}
				}
				else
				{
					if (delta % 9 == 0)
					{
						while (dest % 8 != 0)
						{
							attackMap[dest].set(piece, 0);
							if (board[dest] != '-')
								break;
							dest += 9;
						}
					}
					else
					{
						while (dest % 8 != 7 && dest > 0)
						{
							attackMap[dest].set(piece, 0);
							if (board[dest] != '-')
								break;
							dest += 7;
						}
					}
				}
			}
		default:
			break;
		}
		dest = sq.getInd();
		attackMap[dest].set(piece, 1);
	}
}

void Position::place(const Square& square, const char piece)
{
	board[square.getInd()] = piece;
	for (auto phase : { MG, EG })
	{
		pieceValuesSum[phase] += pieceValues[PIECES[piece]][phase];
		psqtBonusSum[phase] += PSQT[PIECES[piece]][square.rank][square.file][phase];
	}

	const Color pieceColor = colorOf(piece);
	if (nonPawn(piece))
		nonPawnMaterial[pieceColor] += pieceValues[PIECES[piece]][MG];
	pieces[PIECES[piece]].set(square.getInd());

	updateOccupiedSquare(square);
	updatePiece(square);
	color[pieceColor].set(square.getInd());
}

void Position::remove(const Square& square)
{
	const char removedPiece = board[square.getInd()];
	if (removedPiece == '-')
		return;
	for (auto phase : { MG, EG })
	{
		pieceValuesSum[phase] -= pieceValues[PIECES[removedPiece]][phase];
		psqtBonusSum[phase] -= PSQT[PIECES[removedPiece]][square.rank][square.file][phase];
	}
	board[square.getInd()] = '-';

	const Color pieceColor = colorOf(removedPiece);
	if (nonPawn(removedPiece))
		nonPawnMaterial[pieceColor] -= pieceValues[PIECES[removedPiece]][MG];
	pieces[PIECES[removedPiece]].reset(square.getInd());

	updateEmptySquare(square);
	updatePiece(square);
	color[pieceColor].reset(square.getInd());
}

void Position::doMove(const Move& move)
{
	const char pieceToMove = board[move.from.getInd()];
	const char capturedPiece = move.captured;
	Square captureSquare = { move.to.rank, move.to.file };
	remove(move.from);
	switch (move.moveType)
	{
	case DEFAULT:
		if (capturedPiece != '-')
			remove(captureSquare);
		place(move.to, pieceToMove);
		break;
	case EN_PASSANT:
		captureSquare.file = sideToMove ? 3 : 4; // one of the central lines
		remove(captureSquare);
		place(move.to, pieceToMove);
		break;
	case PROMOTION:
		if (capturedPiece != '-')
			remove(captureSquare);
		place(move.to, sideToMove ? 'Q' : 'q');
		break;
	}
	sideToMove = !sideToMove;
}

void Position::undoMove(const Move& move)
{
	sideToMove = !sideToMove;
	const char pieceToMoveBack = board[move.to.getInd()];
	remove(move.to);
	switch (move.moveType)
	{
	case DEFAULT:
		place(move.from, pieceToMoveBack);
		break;
	case EN_PASSANT:
		place(move.from, pieceToMoveBack);
		if (sideToMove)
		{
			place({ 3, move.to.rank }, 'p');
		}
		else
		{
			place({ 4, move.to.rank }, 'P');
		}
		break;
	case PROMOTION:
		place(move.from, sideToMove ? 'P' : 'p');
		break;
	}
	place(move.to, move.captured);
}

std::bitset<64>& Position::pawns(const Color color)
{
	return pieces[color * 6];
}

std::bitset<64>& Position::queen(const Color color)
{
	return pieces[color * 6 + 4];
}

std::bitset<64>& Position::king(const Color color)
{
	return pieces[color * 6 + 5];
}

std::bitset<64> Position::pawnAttacks(const Color color)
{
	std::bitset<64> pawnsPushed = pawns(color);
	if (color == COLOR_W)
		pawnsPushed >>= 8;
	else
		pawnsPushed <<= 8;
	const std::bitset<64> nonEdgePawns = pawnsPushed & ~FILES[0] & ~FILES[7];
	const std::bitset<64> leftEdgePawns = pawnsPushed & FILES[0];
	const std::bitset<64> rightEdgePawns = pawnsPushed & FILES[7];
	return (nonEdgePawns << 1) | (nonEdgePawns >> 1) | (leftEdgePawns << 1) | (rightEdgePawns >> 1);
}

extern Position position = Position();