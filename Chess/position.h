#pragma once
#include "move.h"
#include <fstream>
#include <bitset>
#include <array>
#include <string>

inline bool getColor(char& piece)
{
	return piece <= 'Z';
}

inline square findSquare(const std::bitset<64>& bitset) // function that finds '1' in bitset
{
	for (int i; i < bitset.size(); ++i)
	{
		if (bitset.test(i))
			return i;
	}
	return 64;
}

struct Position {
	std::string board;
	unsigned char sideToMove;
	square enPassantSquare;

	std::array<Move, 256> moves;
	unsigned char movesSize = 0;

	std::array<std::bitset<64>, 64> attackMap;
	std::array<std::bitset<64>, 2> color;
	std::array<int, COLOR_NONE> nonPawnMaterial; // = ... (starting non-pawn material)

	// tapered evaluation data

	std::array<int, PHASE_NONE> pieceValuesSum = {0, 0};
	std::array<int, PHASE_NONE> psqtBonusSum = {0, 0};

	char& operator[](const size_t ind) { return board[ind]; }
	const char& operator[](const size_t ind) const { return board[ind]; }
	inline void doMove(const Move& move);
	inline void undoMove(const Move& move);

	inline void updateAllAttackers(const Square& square);

	inline void initAttackMap()
	{
		std::ifstream attackMapTXT;
		attackMapTXT.open("attackMap.txt");
		char isAlreadyInited;
		attackMapTXT >> isAlreadyInited;
		if (isAlreadyInited == '0' || 1)
		{
			for (unsigned char ind = 0; ind < 8; ++ind)
			{
				switch (board[SQUARE(0, ind)])
				{
				case 'r':
					if (ind > 0)
					{
						attackMap[SQUARE(0, ind - 1)].set(SQUARE(0, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(0, ind + 1)].set(SQUARE(0, ind));
					}
					attackMap[SQUARE(1, ind)].set(SQUARE(0, ind));
					break;
				case 'n':
					if (ind > 0)
					{
						attackMap[SQUARE(2, ind - 1)].set(SQUARE(0, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(2, ind + 1)].set(SQUARE(0, ind));
					}
					break;
				case 'b':
					if (ind > 0)
					{
						attackMap[SQUARE(1, ind - 1)].set(SQUARE(0, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(1, ind + 1)].set(SQUARE(0, ind));
					}
					break;
				case 'q':
				case 'k':
					if (ind > 0)
					{
						attackMap[SQUARE(0, ind - 1)].set(SQUARE(0, ind));
						attackMap[SQUARE(1, ind - 1)].set(SQUARE(0, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(0, ind + 1)].set(SQUARE(0, ind));
						attackMap[SQUARE(1, ind + 1)].set(SQUARE(0, ind));
					}
					attackMap[SQUARE(1, ind)].set(SQUARE(0, ind));
					break;
				default:
					break;
				}

				switch (board[SQUARE(7, ind)])
				{
				case 'R':
					if (ind > 0)
					{
						attackMap[SQUARE(7, ind - 1)].set(SQUARE(7, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(7, ind + 1)].set(SQUARE(7, ind));
					}
					attackMap[SQUARE(6, ind)].set(SQUARE(7, ind));
					break;
				case 'N':
					if (ind > 0)
					{
						attackMap[SQUARE(5, ind - 1)].set(SQUARE(7, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(5, ind + 1)].set(SQUARE(7, ind));
					}
					break;
				case 'B':
					if (ind > 0)
					{
						attackMap[SQUARE(6, ind - 1)].set(SQUARE(7, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(6, ind + 1)].set(SQUARE(7, ind));
					}
					break;
				case 'Q':
				case 'K':
					if (ind > 0)
					{
						attackMap[SQUARE(7, ind - 1)].set(SQUARE(7, ind));
						attackMap[SQUARE(6, ind - 1)].set(SQUARE(7, ind));
					}
					if (ind < 7)
					{
						attackMap[SQUARE(7, ind + 1)].set(SQUARE(7, ind));
						attackMap[SQUARE(6, ind + 1)].set(SQUARE(7, ind));
					}
					attackMap[SQUARE(6, ind)].set(SQUARE(7, ind));
					break;
				default:
					break;
				}

				//Pawn init
				if (ind > 0)
				{
					attackMap[SQUARE(2, ind - 1)].set(SQUARE(1, ind));
					attackMap[SQUARE(5, ind - 1)].set(SQUARE(6, ind));
				}
				if (ind < 7)
				{
					attackMap[SQUARE(2, ind + 1)].set(SQUARE(1, ind));
					attackMap[SQUARE(5, ind + 1)].set(SQUARE(6, ind));
				}
				attackMap[SQUARE(2, ind)].set(SQUARE(1, ind));
				attackMap[SQUARE(3, ind)].set(SQUARE(1, ind));
				attackMap[SQUARE(5, ind)].set(SQUARE(6, ind));
				attackMap[SQUARE(4, ind)].set(SQUARE(6, ind));
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

	inline void init(std::string FEN)
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
		auto enPassantTargetColumn = *current;
		++current;
		auto enPassantTargetRow = *current;
		enPassantSquare = SQUARE(enPassantTargetColumn - 'a' + 1, enPassantTargetRow - '0');
		initAttackMap();
	}

	inline void logAttackMap()
	{
		std::ofstream  attackMapTXT;
		attackMapTXT.open("attackMap.txt", std::ofstream::out | std::ofstream::trunc);
		attackMapTXT << 1 << std::endl;
		for (int dest = 0; dest < 64; ++dest)
		{
			attackMapTXT << dest << std::endl;
			auto str = attackMap[dest].to_string();
			std::reverse(str.begin(), str.end());
			for(int c=0; c<64;++c)
			{
				if (c % 8 == 0)
					attackMapTXT << std::endl;
				attackMapTXT << str[c];
			}
			attackMapTXT << std::endl << attackMap[dest].to_string() << std::endl;

		}
		attackMapTXT.close();
	}
};

extern Position position;




