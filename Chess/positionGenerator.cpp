#include "positionGenerator.h"

inline void generateMoves()
{
	position.movesSize = 0;

	std::bitset<64> movablePieces;

	square piece;

	for (square dest = 0; dest < 64; dest++)
	{
		movablePieces = position.attackMap[dest] & position.color[position.sideToMove]; // picks from all pieces of color[sideToMove] and pieces which attack dest
		while (movablePieces.any())
		{
			piece = findSquare(movablePieces);

			if (getColor(position.board[dest]) == position.sideToMove) //trying to eat a figure of the same color
			{
				movablePieces.set(piece, 0);
				continue;
			}

			if (position.board[piece] == 'p' || position.board[piece] == 'P') // if piece is a pawn
			{
				if (dest == position.enPassantSquare) //en passant
				{
					position.moves[position.movesSize++] = Move({ Square(piece), Square(dest), EN_PASSANT, position.board[dest] });
					movablePieces.set(piece, 0);
					continue;
				}
				if (dest - piece == 8 || dest - piece == -8)
				{
					if (position.board[dest] != '-')
					{
						movablePieces.set(piece, 0);
						continue;
					}
				}
				if (dest - piece == 16) // double square move 
				{
					if (position.board[dest + 8] != '-' || position.board[dest] != '-')
					{
						movablePieces.set(piece, 0);
						continue;
					}
					position.moves[position.movesSize++] = Move({ Square(piece), Square(dest), DEFAULT, '-' });
					movablePieces.set(piece, 0);
					continue;
				}
				if (dest - piece == -16) // double square move 
				{
					if (position.board[dest - 8] != '-' || position.board[dest] != '-')
					{
						movablePieces.set(piece, 0);
						continue;
					}
					position.moves[position.movesSize++] = Move({ Square(piece), Square(dest), DEFAULT, '-' });
					movablePieces.set(piece, 0);
					continue;
				}
				if (dest < 8 || dest >= 56)
				{
					position.moves[position.movesSize++] = Move({ Square(piece), Square(dest), PROMOTION, position.board[dest] });
					movablePieces.set(piece, 0);
					continue;
				}
			}

			position.moves[position.movesSize++] = Move({ Square(piece), Square(dest), DEFAULT, position.board[dest] });
			movablePieces.set(piece, 0);
			continue;
		}
	}
}
