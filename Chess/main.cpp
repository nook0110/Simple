#include <iostream>
#include <array>
#include <string>
#include "globals.h"
#undef class
#undef new
#undef struct
#undef delete


// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 0 1


void init(const std::string& FEN)
{
	auto current = FEN.begin();
	for (int row = 7; row >= 0; --row, ++current)
	{
		for (int column = 0; current != FEN.end() && *current != '/' && column < 8; ++current)
		{
			if (*current >= '0' && *current <= '9')
			{
				column += *current - '1';
			}
			else
			{
				board[row][column] = *current;
			}

			++column;
		}
		if (current == FEN.end())
		{
			break;
		}
	}
	info = FEN.substr(current - FEN.begin());
}

int main()
{
	std::string FEN;
	std::getline(std::cin, FEN);
	init(FEN);
	return 0;
}