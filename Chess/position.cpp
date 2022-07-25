#include "position.h"
#include "attacks.h"
#include "pieceSquareTables.hpp"

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

	pieces[PIECES[piece]] |= SQUAREBB(square.getInd());
	color[pieceColor] |= SQUAREBB(square.getInd());

	// hash key updating
	//hash ^= psq_keys[PIECES[piece]][square.getInd()];
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
	
	pieces[PIECES[removedPiece]] &= ~SQUAREBB(square.getInd());
	color[pieceColor] &= ~SQUAREBB(square.getInd());

	// hash key updating
	//hash ^= psq_keys[PIECES[removedPiece]][square.getInd()];
}

void Position::doMove(const Move& move)
{
	const char pieceToMove = board[move.from.getInd()];
	const char capturedPiece = move.captured;
	Square captureSquare = { move.to.rank, move.to.file };
	remove(move.from);
	if (enPassantSquare != -1)
	{
		//hash ^= enpass[enPassantSquare & 7];
		enPassantSquare = -1;
	}
	switch (move.moveType)
	{
	case DOUBLE:
		place(move.to, pieceToMove);
		enPassantSquare = (move.to.getInd() + move.from.getInd()) >> 1;
		//hash ^= enpass[move.from.file];
		break;
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

	//hash ^= sideToMoveKey;
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

	//hash ^= sideToMoveKey;
}

const bitboard Position::pawns(const Color color) const
{
	return pieces[shift[color] + PAWN];
}

const bitboard Position::queen(const Color color) const
{
	return pieces[shift[color] + QUEEN];
}

const bitboard Position::king(const Color color) const
{
	return pieces[shift[color] + KING];
}

bitboard Position::pawnAttacks(const Color color) const
{
	bitboard pawnsPushed = pawns(color);
	if (color == COLOR_W)
		pawnsPushed >>= 8;
	else
		pawnsPushed <<= 8;
	const bitboard nonEdgePawns = pawnsPushed & ~(FILES[0] | FILES[7]);
	const bitboard leftEdgePawns = pawnsPushed & FILES[0];
	const bitboard rightEdgePawns = pawnsPushed & FILES[7];
	return (nonEdgePawns << 1) | (nonEdgePawns >> 1) | (leftEdgePawns << 1) | (rightEdgePawns >> 1);
}

const bool Position::underCheck(const Color us) const
{
	const Color them = flip(us);
	const square kp = kingPos[us];
	const bitboard all = color[us] | color[them];
	if (pawnAttacks(them) & SQUAREBB(kp))
		return true;
	for (auto piece : { KNIGHT, BISHOP, ROOK, QUEEN, KING })
	{
		if (attack_map(piece, kp, all) & pieces[piece + shift[them]])
			return true;
	}
	return false;
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

extern Position position = Position();