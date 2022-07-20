#include "position.h"

std::vector<Move> Position::generateMoves()
{
	std::vector<Move> moves;
	moves.reserve(256);
	std::bitset<64> movablePieces;

	square piece;

	for (square dest = 0; dest < 64; dest++)
	{
		piece = 0;
		movablePieces = attackMap[dest] & color[sideToMove]; // picks from all pieces of color[sideToMove] and pieces which attack dest
		while (movablePieces.any())
		{
			piece = findSquare(movablePieces, piece);
			movablePieces.set(piece, 0);
			if (colorOf(board[dest]) == sideToMove) //trying to eat a figure of the same color
			{
				continue;
			}	

			if (board[piece] == 'p' || board[piece] == 'P') // if piece is a pawn
			{
				if (dest == enPassantSquare) //en passant
				{
					moves.push_back(Move({ Square(piece), Square(dest), EN_PASSANT, board[dest] }));
					continue;
				}
				square delta = dest - piece;
				if (dest < 8 || dest >= 56)
				{
					if (delta == 8 || delta == -8 && board[dest]=='-')
					{
						moves.push_back(Move({ Square(piece), Square(dest), PROMOTION, '-'}));
						continue;
					}
					if (board[dest] == '-')
					{
						continue;
					}
					moves.push_back(Move({ Square(piece), Square(dest), PROMOTION, board[dest]}));
					continue;
				}
				if (dest - piece == 8 || dest - piece == -8)
				{
					if (board[dest] != '-')
					{
						continue;
					}
					moves.push_back(Move({ Square(piece), Square(dest), DEFAULT, '-' }));
					continue;
				}
				if (dest - piece == 16) // double square move 
				{
					if (board[dest - 8] != '-' || board[dest] != '-')
					{
						continue;
					}
					moves.push_back(Move({ Square(piece), Square(dest), DEFAULT, '-' }));
					continue;
				}
				if (dest - piece == -16) // double square move 
				{
					if (board[dest + 8] != '-' || board[dest] != '-')
					{
						continue;
					}
					moves.push_back(Move({ Square(piece), Square(dest), DEFAULT, '-' }));
					continue;
				}
				if (board[dest] == '-')
				{
					continue;
				}
			}

			moves.push_back(Move({ Square(piece), Square(dest), DEFAULT, board[dest] }));
			continue;
		}
	}
	return moves;
}
