#include "position.h"
#include "pieceSquareTables.hpp"

inline void  Position::updateAllAttackers(const Square& sq)
{
	auto attackers = attackMap[sq.getInd()];

	square piece;
	while (attackers.any())
	{

	}

}

inline void Position::doMove(const Move& move)
{
	const char pieceToMove = board[move.from.getInd()];
	for (const auto phase : {MG, EG}) psqtBonusSum[phase] -= PSQT[PIECES[pieceToMove]][move.from.x][move.from.y][phase];
	const char capturedPiece = move.captured;
	Square captureSquare = { move.to.x, move.to.y };
	switch (move.moveType)
	{
	case DEFAULT:
		board[move.to.getInd()] = pieceToMove;
		for (const auto phase : {MG, EG}) psqtBonusSum[phase] += PSQT[PIECES[pieceToMove]][move.to.x][move.to.y][phase];
		break;
	case EN_PASSANT:
		board[move.to.getInd()] = pieceToMove;
		board[SQUARE(move.to.x, sideToMove ? 3 : 4)] = '-';
		captureSquare.y = sideToMove ? 3 : 4;
		for (const auto phase : {MG, EG}) psqtBonusSum[phase] += PSQT[PIECES[pieceToMove]][move.to.x][move.to.y][phase];
		break;
	case PROMOTION:
		board[move.to.getInd()] = (sideToMove ? 'Q' : 'q');
		for (const auto phase : {MG, EG})
		{
			// substract value of the promoted pawn, add new queen value
			pieceValuesSum[phase] -= pieceValues[PIECES[pieceToMove]][phase];
			pieceValuesSum[phase] += pieceValues[PIECES[sideToMove ? 'Q' : 'q']][phase];
			psqtBonusSum[phase] += PSQT[PIECES[sideToMove ? 'Q' : 'q']][move.to.x][move.to.y][phase];
		}
		break;
	}
	board[move.from.getInd()] = '-';
	for (auto phase : {MG, EG})
	{
		// substract value of the captured piece
		pieceValuesSum[phase] -= pieceValues[PIECES[capturedPiece]][phase];
		psqtBonusSum[phase] -= PSQT[PIECES[capturedPiece]][captureSquare.x][captureSquare.y][phase];
		// substract bonus value of the moved piece
		psqtBonusSum[phase] -= PSQT[PIECES[pieceToMove]][move.from.x][move.from.y][phase];
	}
	sideToMove = !sideToMove;
}

inline void Position::undoMove(const Move& move)
{
	sideToMove = !sideToMove;
	switch (move.moveType)
	{
	case DEFAULT:
		board[move.from.getInd()] = board[move.to.getInd()];
		break;
	case EN_PASSANT:
		board[move.from.getInd()] = board[move.to.getInd()];
		if (sideToMove)
		{
			board[SQUARE(3, move.to.x)] = 'p';
		}
		else
		{
			board[SQUARE(4, move.to.x)] = 'P';
		}
		break;
	case PROMOTION:
		board[move.from.getInd()] = (sideToMove ? 'P' : 'p');
		break;
	}
	board[move.to.getInd()] = move.captured;
}

extern Position position = { std::string("----------------------------------------------------------------") };