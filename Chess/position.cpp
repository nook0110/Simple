#include "position.h"

inline void Position::doMove(const Move& move)
{
	switch (move.moveType)
	{
	case DEFAULT:
		board[move.to.getInd()] = board[move.from.getInd()];
		break;
	case EN_PASSANT:
		board[move.to.getInd()] = board[move.from.getInd()];
		board[SQUARE(sideToMove ? 3 : 4, move.to.x)] = '-';
		break;
	case PROMOTION:
		board[move.to.getInd()] = (sideToMove ? 'Q' : 'q');
		break;
	}
	board[move.from.getInd()] = '-';
	sideToMove = !sideToMove;
}

inline void Position::initAttackMap()
{
	std::ifstream attackMapTXT;
	attackMapTXT.open("attackMap.txt");
	char isAlreadyInited;
	attackMapTXT >> isAlreadyInited;
	if (isAlreadyInited == '0')
	{
		for (unsigned char ind = 0; ind < 8; ++ind)
		{
			if (board[SQUARE(0, ind)] == 'n')
			{
				if (ind > 0)
				{
					attackMap[SQUARE(2, ind - 1)].set(SQUARE(0, ind));
				}
				if (ind < 7)
				{
					attackMap[SQUARE(2, ind + 1)].set(SQUARE(0, ind));
				}
			}
			if (board[SQUARE(7, ind)] == 'N')
			{
				if (ind > 0)
				{
					attackMap[SQUARE(6, ind - 1)].set(SQUARE(7, ind));
				}
				if (ind < 7)
				{
					attackMap[SQUARE(6, ind + 1)].set(SQUARE(7, ind));
				}
			}
			if (ind > 0)
			{
				attackMap[SQUARE(2, ind - 1)].set(SQUARE(1, ind));
				attackMap[SQUARE(6, ind - 1)].set(SQUARE(7, ind));
			}
			if (ind < 7)
			{
				attackMap[SQUARE(2, ind + 1)].set(SQUARE(1, ind));
				attackMap[SQUARE(6, ind + 1)].set(SQUARE(7, ind));
			}
			attackMap[SQUARE(2, ind)].set(SQUARE(1, ind));
			attackMap[SQUARE(3, ind)].set(SQUARE(1, ind));
			attackMap[SQUARE(6, ind)].set(SQUARE(7, ind));
			attackMap[SQUARE(5, ind)].set(SQUARE(7, ind));
		}
	}
	else
	{
		std::string bitset;
		for (int i = 0; i < 64; ++i)
		{
			attackMapTXT >> bitset;
			attackMap[i] = std::bitset<64>(bitset);
		}
	}
}

void Position::logAttackMap()
{
	std::ofstream  attackMapTXT;
	attackMapTXT.open("attackMap.txt", std::ofstream::out | std::ofstream::trunc);
	attackMapTXT << 1 << std::endl;
	for (int dest = 0; dest < 64; ++dest)
	{
		/*attackMapTXT << dest << std::endl;
		auto str = attackMap[dest].to_string();
		for(int c=0; c<64;++c)
		{
			if (c % 8 == 0)
				attackMapTXT << std::endl;
			attackMapTXT << str[c];
		}
		*/
		attackMapTXT << attackMap[dest].to_string() << std::endl;

	}
	attackMapTXT.close();
}

void Position::init(std::string FEN)
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
				color[getColor(*current)] = 1;
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
	enPassantTargetColumn = *current;
	++current;
	enPassantTargetRow = *current;
	initAttackMap();
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
	board[move.to.getInd()] = move.pieceTo;
}

extern Position position = { std::string("----------------------------------------------------------------") };