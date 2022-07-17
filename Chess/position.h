#pragma once
#pragma once
#include "move.h"
#include "eval.h"
#include <fstream>
#include <bitset>
#include <array>
#include <string>

inline bool getColor(const char piece)
{
	return piece <= 'Z';
}

inline square findSquare(const std::bitset<64>& bitset) // function that finds '1' in bitset
{
	for (square i = 0; i < bitset.size(); ++i)
	{
		if (bitset.test(i))
			return i;
	}
	return 64;
}

struct Position {
	std::string board = "----------------------------------------------------------------";
	unsigned char sideToMove;
	square enPassantSquare;

	std::array<std::bitset<64>, 64> attackMap;
	std::array<std::bitset<64>, COLOR_NONE> color;
	std::array<std::bitset<64>, 12> pieces;
	std::array<int, COLOR_NONE> nonPawnMaterial = { 8302, -8302 };

	// tapered evaluation data

	std::array<int, PHASE_NONE> pieceValuesSum = { 0, 0 };
	std::array<int, PHASE_NONE> psqtBonusSum = { 0, 0 };

	char& operator[](const size_t ind) { return board[ind]; }
	const char& operator[](const size_t ind) const { return board[ind]; }

	// helpers

	void place(const Square& square, const char piece);
	void remove(const Square& square);

	void doMove(const Move& move);
	void undoMove(const Move& move);

	void updatePiece(const Square& square);
	void updateEmptySquare(const Square& square);
	void updateOccupiedSquare(const Square& square);

	std::bitset<64>& pawns(const Color color);
	std::bitset<64>& queen(const Color color);
	std::bitset<64>& king(const Color color);
	std::bitset<64> pawnAttacks(const Color color);

	// evaluation

	std::array<std::bitset<64>, COLOR_NONE> avaliableArea;
	std::array<std::array<value, PHASE_NONE>, COLOR_NONE> mobility;
	std::array<value, PHASE_NONE> eval = { 0, 0 };

	const value evaluate();

	void initAttackMap()
	{
		std::ifstream attackMapTXT;
		attackMapTXT.open("attackMap.txt");
		char isAlreadyInited;
		attackMapTXT >> isAlreadyInited;
		if (isAlreadyInited == '0' || 1)
		{
			for (square ind = 0; ind < 64; ++ind)
			{
				if (board[ind] == '-')
					continue;
				place(Square(ind), board[ind]);
			}
			/*
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
			*/
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

	void init(std::string FEN)
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

	void logAttackMap()
	{
		std::ofstream  attackMapTXT;
		attackMapTXT.open("attackMap.txt", std::ofstream::out | std::ofstream::trunc);
		attackMapTXT << 1 << std::endl;
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

	std::pair<value, value> findAlphaBeta(int depth, Move previous = Move());
};

extern Position position;