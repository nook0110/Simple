#include "position.h"
#include "attacks.h"
#include "pieceSquareTables.hpp"

void Position::place(const Square& square, const Piece piece)
{
	if (piece == EMPTY)
	{
		remove(square);
		return;
	}
	if (piece == KING_B)
	{
		kingPos[COLOR_B] = square.getInd();
	}
	if (piece == KING_W)
	{
		kingPos[COLOR_W] = square.getInd();
	}
	board[square.getInd()] = piece;
	for (auto phase : { MG, EG })
	{
		pieceValuesSum[phase] += pieceValues[piece][phase];
		psqtBonusSum[phase] += PSQT[piece][square.rank][square.file][phase];
	}

	const Color pieceColor = colorOf(piece);
	if (nonPawn(piece))
		nonPawnMaterial[pieceColor] += pieceValues[piece][MG];

	pieces[piece] |= SQUAREBB(square.getInd());
	color[pieceColor] |= SQUAREBB(square.getInd());

	// hash key updating
	hash ^= psq_keys[piece][square.getInd()];
}

void Position::remove(const Square& square)
{
	const auto removedPiece = board[square.getInd()];
	if (removedPiece == EMPTY)
		return;
	for (auto phase : { MG, EG })
	{
		pieceValuesSum[phase] -= pieceValues[removedPiece][phase];
		psqtBonusSum[phase] -= PSQT[removedPiece][square.rank][square.file][phase];
	}
	board[square.getInd()] = EMPTY;

	const Color pieceColor = colorOf(removedPiece);
	if (nonPawn(removedPiece))
		nonPawnMaterial[pieceColor] -= pieceValues[removedPiece][MG];
	
	pieces[removedPiece] &= ~SQUAREBB(square.getInd());
	color[pieceColor] &= ~SQUAREBB(square.getInd());

	// hash key updating
	hash ^= psq_keys[removedPiece][square.getInd()];
}

void Position::doMove(const Move& move)
{
	const auto pieceToMove = board[move.from.getInd()];
	const auto capturedPiece = move.captured;
	Square captureSquare = { move.to.rank, move.to.file };
	remove(move.from);
	if (enPassantSquare != -1)
	{
		//hash ^= enpass[enPassantSquare & 7];
		enPassantSquare = -1;
	}
	if (sideToMove == COLOR_W)
	{
		if (pieceToMove == KING_W)
			cr[COLOR_W] = CR_NONE;
		else if (pieceToMove == ROOK_W && move.from.rank == 7 && move.from.file == 0)
			cr[COLOR_W] = static_cast<CastlingRight>(cr[COLOR_W] & ~CR_OOO);
		else if (pieceToMove == ROOK_W && move.from.rank == 7 && move.from.file == 7)
			cr[COLOR_W] = static_cast<CastlingRight>(cr[COLOR_W] & ~CR_OO);
		if (capturedPiece == ROOK_B)
		{
			if (move.to.rank == 0 && move.to.file == 0)
				cr[COLOR_B] = static_cast<CastlingRight>(cr[COLOR_B] & ~CR_OOO);
			if (move.to.rank == 0 && move.to.file == 7)
				cr[COLOR_B] = static_cast<CastlingRight>(cr[COLOR_B] & ~CR_OO);
		}

	}
	else
	{
		if (pieceToMove == KING_B)
			cr[COLOR_B] = CR_NONE;
		else if (pieceToMove == ROOK_B && move.from.rank == 0 && move.from.file == 0)
			cr[COLOR_B] = static_cast<CastlingRight>(cr[COLOR_B] & ~CR_OOO);
		else if (pieceToMove == ROOK_B && move.from.rank == 0 && move.from.file == 7)
			cr[COLOR_B] = static_cast<CastlingRight>(cr[COLOR_B] & ~CR_OO);
		if (capturedPiece == ROOK_W)
		{
			if (move.to.rank == 7 && move.to.file == 0)
				cr[COLOR_W] = static_cast<CastlingRight>(cr[COLOR_W] & ~CR_OOO);
			if (move.to.rank == 7 && move.to.file == 7)
				cr[COLOR_W] = static_cast<CastlingRight>(cr[COLOR_W] & ~CR_OO);
		}
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
		place(move.to, sideToMove ? QUEEN_W : QUEEN_B);
		break;
	case CASTLING_OO:
		// king moves
		place(move.to, pieceToMove);
		// rook moves
		remove(SQUARE(sideToMove ? 7 : 0, 7));
		place(SQUARE(sideToMove ? 7 : 0, 5), sideToMove ? ROOK_W : ROOK_B);
		break;
	case CASTLING_OOO:
		// king moves
		place(move.to, pieceToMove);
		// rook moves
		remove(SQUARE(sideToMove ? 7 : 0, 0));
		place(SQUARE(sideToMove ? 7 : 0, 3), sideToMove ? ROOK_W : ROOK_B);
		break;
	}
	sideToMove = !sideToMove;

	hash ^= sideToMoveKey;
}

void Position::undoMove(const Move& move)
{
	sideToMove = !sideToMove;
	const auto pieceToMoveBack = board[move.to.getInd()];
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
			place({ 3, move.to.rank }, PAWN_B);
		}
		else
		{
			place({ 4, move.to.rank }, PAWN_W);
		}
		break;
	case PROMOTION:
		place(move.from, sideToMove ? PAWN_W : PAWN_B);
		break;
	case CASTLING_OO:
		// king moves back
		place(move.from, pieceToMoveBack);
		// rook moves back
		remove(SQUARE(sideToMove ? 7 : 0, 5));
		place(SQUARE(sideToMove ? 7 : 0, 7), sideToMove ? ROOK_W : ROOK_B);
		break;
	case CASTLING_OOO:
		// king moves back
		place(move.from, pieceToMoveBack);
		// rook moves back
		remove(SQUARE(sideToMove ? 7 : 0, 3));
		place(SQUARE(sideToMove ? 7 : 0, 0), sideToMove ? ROOK_W : ROOK_B);
		break;
	}
	if (move.captured != EMPTY)
		place(move.to, move.captured);

	hash ^= sideToMoveKey;
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

const bool Position::underAttack(const square sq, const Color us) const
{
	const Color them = flip(us);
	const bitboard all = color[us] | color[them];
	if (pawnAttacks(them) & SQUAREBB(sq))
		return true;
	for (auto piece : { KNIGHT, BISHOP, ROOK, QUEEN, KING })
	{
		if (attack_map(piece, sq, all) & pieces[piece + shift[them]])
			return true;
	}
	return false;
}

const bool Position::underCheck(const Color us) const
{
	return underAttack(kingPos[us], us);
}

const bool Position::canCastle(const Color us, const CastlingRight cr) const
{
	if (!(this->cr[us] & cr))
		return false;
	if (castlingPath[us][cr] & (color[us] | color[flip(us)]))
		return false;
	if (underCheck(us))
		return false;
	if (cr & CR_OO)
	{
		for (auto sq = kingPos[us] + 1; sq <= kingDestination[us][CR_OO]; ++sq)
		{
			if (underAttack(sq, us))
				return false;
		}
	}
	if (cr & CR_OOO)
	{
		for (auto sq = kingPos[us] - 1; sq >= kingDestination[us][CR_OOO]; --sq)
		{
			if (underAttack(sq, us))
				return false;
		}
	}
	return true;
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
				board[SQUARE(row, column)] = PIECES[*current];
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
		if (board[ind] == EMPTY)
			continue;
		place(Square(ind), board[ind]);
	}
	initCastlingPath();
}

void Position::initCastlingPath()
{
	castlingPath[COLOR_W].fill(0);
	castlingPath[COLOR_B].fill(0);
	for (auto us : { COLOR_W, COLOR_B })
	{
		for (auto sq = kingPos[us] + 1; (sq & 7) < 7; ++sq)
			castlingPath[us][CR_OO]  |= SQUAREBB(sq);
		for (auto sq = kingPos[us] - 1; (sq & 7) > 0; --sq)
			castlingPath[us][CR_OOO] |= SQUAREBB(sq);
	}
}

extern Position position = Position();