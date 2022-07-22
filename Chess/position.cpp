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
	square piece = 0;
	while (attackers.any())
	{
		dest = sq.getInd();
		piece = findSquare(attackers, piece);
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
	square piece = 0;
	auto type = board[dest];
	board[dest] = '-';
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
					while (dest % 8 != 0 && dest >= 0)
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
					while (dest % 8 != 0 && dest < 64)
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
			break;
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
						while (dest % 8 != 0 && dest >= 0)
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
						while (dest % 8 != 0 && dest < 64)
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
			break;
		default:
			break;
		}
		dest = sq.getInd();
		attackMap[dest].set(piece, 1);
	}
	board[dest] = type;
}

void Position::place(const Square& square, const char piece)
{
	if (piece == '-')
	{
		remove(square);
		return;
	}
	if (piece == 'k')
	{
		kingPos[COLOR_B] = square.getInd();
	}
	if (piece == 'K')
	{
		kingPos[COLOR_W] = square.getInd();
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
	case DOUBLE:
		place(move.to, pieceToMove);
		enPassantSquare = (move.to.getInd() + move.from.getInd()) / 2;
		break;
	case DEFAULT:
		enPassantSquare = -1;
		if (capturedPiece != '-')
			remove(captureSquare);
		place(move.to, pieceToMove);
		break;
	case EN_PASSANT:
		enPassantSquare = -1;
		captureSquare.file = sideToMove ? 3 : 4; // one of the central lines
		remove(captureSquare);
		place(move.to, pieceToMove);
		break;
	case PROMOTION:
		enPassantSquare = -1;
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

const std::bitset<64>& Position::pawns(const Color color) const
{
	return pieces[shift[color] + PAWN];
}

const std::bitset<64>& Position::queen(const Color color) const
{
	return pieces[shift[color] + QUEEN];
}

const std::bitset<64>& Position::king(const Color color) const
{
	return pieces[shift[color] + KING];
}

std::bitset<64> Position::pawnAttacks(const Color color) const
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

const bool Position::underCheck(const Color us) const
{
	const Color them = flip(us);
	return (attackMap[kingPos[us]] & (color[them] ^ pawns(them))).any() || (king(us) & pawnAttacks(them)).any();
}

void Position::init(std::string FEN, std::string move)
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
	enPassantSquare = SQUARE(8 - (enPassantTargetRow - '0'), enPassantTargetColumn - 'a');
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

std::mutex updateBM;

void findMove(Position& pos, const std::vector<Move>& moves, value& alpha, value& beta, Move& bestMove)
{
	for (int i = 0; i < moves.size(); ++i)
	{
		pos.doMove(moves[i]);
		auto tempAlpha = pos.findAlphaBeta(1, alpha, beta, moves[i]);
		pos.undoMove(moves[i]);
		if (!tempAlpha.has_value())
		{
			continue;
		}
		if (alpha < tempAlpha.value())
		{
			updateBM.lock();
			bestMove = moves[i];
			alpha = tempAlpha.value();
			updateBM.unlock();
		}
		if (alpha >= beta)
			break;

	}
}

Move Position::findBestMove()
{
	auto moves = generateMoves();
	Move bestMove = moves[0];
	value alpha = INT_MIN;
	value beta = INT_MAX;
	Position c1 = *this, c2 = *this, c3 = *this, c4 = *this, c5 = *this, c6 = *this, c7 = *this;
	auto shift = moves.size() / 8;
	auto remainder = moves.size() % 8;
	std::array<std::vector<Move>::iterator, 9> its;
	
	its[0] = moves.begin();
	for (unsigned long long i = 1; i < 8; ++i)
	{
		its[i] = moves.begin() + shift*i;
		its[i] += std::min(i, remainder);
	}
	its[8] = moves.end();
	for (int i = 0; i < 8; ++i)
	{
		std::sort(its[i], its[i + 1]);
	}

#pragma omp parallel sections num_threads(8)
	{
#pragma omp section
		{
			std::cout << 1;
			findMove(*this, std::vector<Move>(its[0], its[1]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			std::cout << 2;
			findMove(c1, std::vector<Move>(its[1], its[2]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			std::cout << 3;
			findMove(c2, std::vector<Move>(its[2], its[3]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			std::cout << 4;
			findMove(c3, std::vector<Move>(its[3], its[4]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			std::cout << 5;
			findMove(c4, std::vector<Move>(its[4], its[5]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			std::cout << 6;
			findMove(c5, std::vector<Move>(its[5], its[6]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			std::cout << 7;
			findMove(c6, std::vector<Move>(its[6], its[7]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			std::cout << 8;
			findMove(c7, std::vector<Move>(its[7], its[8]), alpha, beta, bestMove);
		}
	}
	//std::cout << alpha << std::endl;

	return bestMove;
}

std::optional<value> Position::findAlphaBeta(int depth, value alpha, value beta, const Move& previous)
{
	auto us = static_cast<Color>(sideToMove);
	auto them = flip(us);
	if (underCheck(them))
	{
		return std::nullopt;
	}
	if (((depth > 5 && previous.captured == '-') && !underCheck(us)) || (depth > 7))
	{
		return (sideToMove == (depth % 2) ? -evaluate() : evaluate());
	}
	value unc = 0;
	if (depth % 2 == 0)
	{
		std::optional<value> tempAlpha;
		auto moves = generateMoves();
		std::sort(moves.begin(), moves.end());
		for (const auto& move : moves)
		{
			if (alpha >= beta)
				break;
			doMove(move);
			tempAlpha = findAlphaBeta(depth + 1, alpha, beta, move);
			undoMove(move);
			if (!tempAlpha.has_value())
			{
				++unc;
				continue;
			}
			if (alpha < tempAlpha.value())
			{
				alpha = tempAlpha.value();
			}
		}
		if (unc == moves.size())
		{
			if ((attackMap[kingPos[sideToMove]] & (color[!sideToMove] ^ pawns(them))).any() || (king(us) & pawnAttacks(them)).any())
			{
				return -100000 + depth << 4;
			}
			else
			{
				return 0;
			}
		}
		return alpha;
	}
	else
	{
		std::optional<value> tempBeta;
		auto moves = generateMoves();
		std::sort(moves.begin(), moves.end());
		for (auto& move : moves)
		{
			if (alpha >= beta)
				break;
			doMove(move);
			tempBeta = findAlphaBeta(depth + 1, alpha, beta, move);
			undoMove(move);
			if (!tempBeta.has_value())
			{
				++unc;
				continue;
			}
			if (beta > tempBeta.value())
			{
				beta = tempBeta.value();
			}
		}
		if (unc == moves.size())
		{
			if ((attackMap[kingPos[sideToMove]] & (color[!sideToMove] ^ pawns(them))).any() || (king(us) & pawnAttacks(them)).any())
			{
				return 100000 - depth << 4;
			}
			else
			{
				return 0;
			}
		}
		return beta;
	}

}


extern Position position = Position();