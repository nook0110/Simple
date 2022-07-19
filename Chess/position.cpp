#include "position.h"
#include "pieceSquareTables.hpp"
#include <iostream>
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
		while (dest % 8 != 7 && dest >= 0)
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
		while (dest % 8 != 0 && dest >= 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -9;
		dest = piece + delta;
		while (dest % 8 != 7 && dest >= 0)
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
		dest = piece + delta;
		while (dest % 8 != 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -1;
		dest = piece + delta;
		while (dest % 8 != 7 && dest >= 0)
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
		while (dest % 8 != 0 && dest >= 0)
		{
			attackMap[dest].set(piece, 1);
			if (board[dest] != '-')
				break;
			dest += delta;
		}
		delta = -9;
		dest = piece + delta;
		while (dest % 8 != 7 && dest >= 0)
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
		if (piece + 8 < 64)
			attackMap[piece + 8].set(piece, 1);
		if ((piece & 7) != 0)
			if (piece + 7 < 64)
				attackMap[piece + 7].set(piece, 1);
		if ((piece & 7) != 7)
			if (piece + 9 < 64)
				attackMap[piece + 9].set(piece, 1);
		break;
	case 'P':
		if (piece >> 3 == 6)
		{
			attackMap[SQUARE(4, piece & 7)].set(piece, 1);
		}
		if (piece - 8 >= 0)
			attackMap[piece - 8].set(piece, 1);
		if ((piece & 7) != 7)
			if (piece - 7 >= 0)
				attackMap[piece - 7].set(piece, 1);
		if ((piece & 7) != 0)
			if (piece - 9 >= 0)
				attackMap[piece - 9].set(piece, 1);
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
	square piece=0;
	while (attackers.any())
	{
		dest = sq.getInd();
		piece = findSquare(attackers,piece);
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
					while (dest % 8 != 7 && dest >= 0)
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
					while (dest % 8 != 0 && dest >= 0)
					{
						attackMap[dest].set(piece, 1);
						if (board[dest] != '-')
							break;
						dest -= 7;
					}
				}
				else
				{
					while (dest % 8 != 7 && dest >= 0)
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
					while (dest % 8 != 7 && dest >= 0)
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
						while (dest % 8 != 0 && dest >= 0)
						{
							attackMap[dest].set(piece, 1);
							if (board[dest] != '-')
								break;
							dest -= 7;
						}
					}
					else
					{
						while (dest % 8 != 7 && dest >= 0)
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
	square piece=0;
	while (attackers.any())
	{
		piece = findSquare(attackers, piece);
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
					while (dest % 8 != 7 && dest >= 0)
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
					while (dest % 8 != 7 && dest >= 0)
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
					while (dest % 8 != 7 && dest < 64)
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
					while (dest % 8 != 7 && dest >= 0)
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
						while (dest % 8 != 7 && dest >= 0)
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
						while (dest % 8 != 7 && dest < 64)
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
	if (piece == '-')
	{
		remove(square);
		return;
	}
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
	if (move.captured != '-')
		place(move.to, move.captured);
}

std::bitset<64>& Position::pawns(const Color color)
{
	return pieces[shift[color] + PAWN];
}

std::bitset<64>& Position::queen(const Color color)
{
	return pieces[shift[color] + QUEEN];
}

std::bitset<64>& Position::king(const Color color)
{
	return pieces[shift[color] + KING];
}

std::bitset<64> Position::pawnAttacks(const Color color)
{
	std::bitset<64> pawnsPushed = pawns(color);
	if (color == COLOR_W)
		pawnsPushed >>= 8;
	else
		pawnsPushed <<= 8;
	const std::bitset<64> nonEdgePawns = pawnsPushed & ~(FILES[0] | FILES[7]);
	const std::bitset<64> leftEdgePawns = pawnsPushed & FILES[0];
	const std::bitset<64> rightEdgePawns = pawnsPushed & FILES[7];
	return (nonEdgePawns << 1) | (nonEdgePawns >> 1) | (leftEdgePawns >> 1) | (rightEdgePawns << 1);
}

void Position::init(std::string FEN, std::string move)
{
	std::ifstream initTXT;
	initTXT.open("Init.txt");
	char isAlreadyInited;
	initTXT >> isAlreadyInited;
	initTXT.close();
	if (isAlreadyInited == '0' || 1)
	{
		auto current = FEN.begin();
		for (int row = 0; row < 8; ++row, ++current)
		{
			for (int column = 0; current != FEN.end() && *current != '/' && column < 8; ++current)
			{
				if (*current >= '0' && *current <= '9')
				{
					column += *current - '1';
				}
				else
				{
					board[SQUARE(row, column)] = *current;
				}
				++column;
			}
			if (current == FEN.end())
			{
				break;
			}
		}
		sideToMove = (*current == 'w');
		current += 2;
		auto enPassantTargetColumn = *current;
		++current;
		auto enPassantTargetRow = *current;
		enPassantSquare = SQUARE(enPassantTargetColumn - 'a' + 1, enPassantTargetRow - '0');
		for (square ind = 0; ind < 64; ++ind)
		{
			if (board[ind] == '-')
				continue;
			place(Square(ind), board[ind]);
		}
		for (auto color : { COLOR_W, COLOR_B })
		{
			nonPawnMaterial[color] -= pieceValues[shift[color] + KING][MG];
		}
	}
	else
	{
		read();
		Move _move;
		if (move.size() == 7)
		{
			_move = { SQUARE(8 - (move[1] - '0'), move[0] - 'a'),
			SQUARE(8 - (move[4] - '0'), move[3] - 'a'),
			PROMOTION,
			board[SQUARE(move[3] - 'a' + 1, move[4] - '0')]
			};
		}
		else if (board[SQUARE(move[1] - 'a' + 1, move[0] - '0')] == 'p' || board[SQUARE(move[1] - 'a' + 1, move[0] - '0')] == 'P')
		{

		}
		else
		{
			_move = { SQUARE(8 - (move[1] - '0'), move[0] - 'a'),
			SQUARE(8 - (move[4] - '0'), move[3] - 'a'),
			DEFAULT,
			board[SQUARE(move[3] - 'a' + 1, move[4] - '0')]
			};
		}
	}

}

void Position::read()
{
	readAttackMap();
}

void Position::log()
{
	logAttackMap();
	logColor();
	logPieces();
}

void Position::readAttackMap()
{
	std::ifstream attackMapTXT;
	attackMapTXT.open("AttackMap.txt");
	std::string bitset;
	for (int i = 0; i < 64; ++i)
	{
		attackMapTXT >> bitset;
		attackMap[i] = std::bitset<64>(bitset);
	}
	attackMapTXT.close();
}

void Position::logAttackMap()
{
	std::ofstream  attackMapTXT;
	attackMapTXT.open("attackMap.txt", std::ofstream::out | std::ofstream::trunc);
	for (int dest = 0; dest < 64; ++dest)
	{
		attackMapTXT << dest << std::endl;
		auto str = attackMap[dest].to_string();
		std::reverse(str.begin(), str.end());
		for (int c = 0; c < 64; ++c)
		{
			if (c % 8 == 0)
				attackMapTXT << std::endl;
			attackMapTXT << str[c];
		}
		attackMapTXT << std::endl << attackMap[dest].to_string() << std::endl;

	}
	attackMapTXT.close();
}

void Position::readColor()
{
	std::ifstream colorTXT;
	colorTXT.open("Color.txt");
	std::string bitset;
	for (auto phase : { COLOR_W, COLOR_B })
	{
		colorTXT >> bitset;
		color[phase] = std::bitset<64>(bitset);
	}
	colorTXT.close();
}

void Position::logColor()
{
	std::ofstream  colorTXT;
	colorTXT.open("Color.txt", std::ofstream::out | std::ofstream::trunc);
	for (auto phase : { COLOR_W, COLOR_B })
	{
		colorTXT << color[phase].to_string() << std::endl;
	}
	colorTXT.close();
}

void Position::readPieces()
{
	std::ifstream  piecesTXT;
	piecesTXT.open("Pieces.txt");
	std::string bitset;
	for (auto piece : { PAWN, KNIGHT, BISHOP, ROOK, KING, QUEEN })
	{
		for (auto color : { COLOR_W, COLOR_B })
		{
			piecesTXT >> bitset;
			pieces[shift[color] + piece] = std::bitset<64>(bitset);
		}
	}
	piecesTXT.close();
}

void Position::logPieces()
{
	std::ofstream  piecesTXT;
	piecesTXT.open("Pieces.txt", std::ofstream::out | std::ofstream::trunc);
	for (auto piece : { PAWN, KNIGHT, BISHOP, ROOK, KING, QUEEN })
	{
		for (auto color : { COLOR_W, COLOR_B })
		{
			piecesTXT << pieces[shift[color] + piece].to_string() << std::endl;
		}
	}
	piecesTXT.close();
}


void findMove(Position& pos,const std::vector<Move>& moves, value& alpha, value& beta, Move& bestMove)
{
	for (int i = 0; i < moves.size(); ++i)
	{
		value tempAlpha = pos.findAlphaBeta(1, alpha, beta, moves[i]);
		pos.undoMove(moves[i]);
		if (alpha < tempAlpha)
		{
			bestMove = moves[i];
			alpha = tempAlpha;
		}
		if (alpha >= beta)
			break;

	}
}

Move Position::findBestMove()
{
	Move bestMove;
	auto moves = generateMoves();
	value alpha = INT_MIN;
	value beta = INT_MAX;

	Position c1 = *this, c2 = *this, c3 = *this, c4 = *this, c5 = *this;
	auto shift = moves.size() / 6;
	std::vector<Move>::const_iterator it1 = moves.begin(), it2 = moves.begin() + shift, it3 = moves.begin() + shift * 2, it4 = moves.begin() + shift * 3, it5 = moves.begin() + shift * 4, it6 = moves.begin() + shift * 5, it7 = moves.end();
#pragma omp parallel sections
	{
#pragma omp section
		{
			findMove(*this,std::vector<Move>(it1, it2), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c1, std::vector<Move>(it2, it3), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c2, std::vector<Move>(it3, it4), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c3, std::vector<Move>(it4, it5), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c4, std::vector<Move>(it5, it6), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c5, std::vector<Move>(it6, it7), alpha, beta, bestMove);
		}
	}
/*#pragma omp parallel sections
	{
#pragma omp section
		{

			for (int i = 0; i < moves.size()/6; ++i)
			{
				std::cout << 1;
				value tempAlpha = findAlphaBeta(1, alpha, beta, moves[i]);
				undoMove(moves[i]);
				//std::cout << std::to_string(tempAlpha) + " ";
				if (alpha < tempAlpha)
				{
					bestMove = &moves[i];
					alpha = tempAlpha;
				}
				if (alpha >= beta)
					break;
				
			}
		}
#pragma omp section
		{
			for (int i = moves.size() / 6; i < (moves.size() / 6) * 2; ++i)
			{
				std::cout << 2;
				value tempAlpha = c1.findAlphaBeta(1, alpha, beta, moves[i]);
				c1.undoMove(moves[i]);
				//std::cout << std::to_string(tempAlpha) + " ";
				if (alpha < tempAlpha)
				{
					bestMove = &moves[i];
					alpha = tempAlpha;
				}
				if (alpha >= beta)
					break;
				
			}
		}
#pragma omp section
		{
			for (int i = (moves.size() / 6) * 2; i < (moves.size() / 6) * 3; ++i)
			{
				value tempAlpha = c2.findAlphaBeta(1, alpha, beta, moves[i]);
				c2.undoMove(moves[i]);
				//std::cout << std::to_string(tempAlpha) + " ";
				if (alpha < tempAlpha)
				{
					bestMove = &moves[i];
					alpha = tempAlpha;
				}
				if (alpha >= beta)
					break;
				
			}
		}
#pragma omp section
		{
			for (int i = (moves.size() / 6) * 3; i < (moves.size() / 6) * 4; ++i)
			{
				value tempAlpha = c3.findAlphaBeta(1, alpha, beta, moves[i]);
				c3.undoMove(moves[i]);
				//std::cout << std::to_string(tempAlpha) + " ";
				if (alpha < tempAlpha)
				{
					bestMove = &moves[i];
					alpha = tempAlpha;
				}
				if (alpha >= beta)
					break;
				
			}
		}
#pragma omp section
		{
			for (int i = (moves.size() / 6) * 4; i < (moves.size() / 6) * 5; ++i)
			{
				value tempAlpha = c4.findAlphaBeta(1, alpha, beta, moves[i]);
				c4.undoMove(moves[i]);
				//std::cout << std::to_string(tempAlpha) + " ";
				if (alpha < tempAlpha)
				{
					bestMove = &moves[i];
					alpha = tempAlpha;
				}
				if (alpha >= beta)
					break;
				
			}
		}
#pragma omp section
		{
			for (int i = (moves.size() / 6) * 5; i < moves.size(); ++i)
			{
				value tempAlpha = c5.findAlphaBeta(1, alpha, beta, moves[i]);
				c5.undoMove(moves[i]);
				//std::cout << std::to_string(tempAlpha) + " ";
				if (alpha < tempAlpha)
				{
					bestMove = &moves[i];
					alpha = tempAlpha;
				}
				if (alpha >= beta)
					break;
				
			}
		}
	}*/


	return bestMove;
}

value Position::findAlphaBeta(int depth, value alpha, value beta, const Move& previous)
{
	doMove(previous);
	if ((depth > 5 && previous.captured == '-') || depth > 5)
	{
		return (sideToMove == depth % 2 ? -evaluate() : evaluate());
	}
	if (depth % 2)
	{
		value tempAlpha;
		auto moves = generateMoves();
		for (const auto& move : moves)
		{
			tempAlpha = findAlphaBeta(depth + 1, alpha, beta, move);
			undoMove(move);
			if (alpha < tempAlpha)
			{
				alpha = tempAlpha;
			}
			if (alpha >= beta)
				break;
		}
		return alpha;
	}
	else
	{
		value tempBeta;
		auto moves = generateMoves();
		for (auto& move : moves)
		{
			tempBeta = findAlphaBeta(depth + 1, alpha, beta, move);
			undoMove(move);
			if (beta > tempBeta)
			{
				beta = tempBeta;
			}
			if (alpha >= beta)
				break;
		}
		return beta;
	}

}


extern Position position = Position();