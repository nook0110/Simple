#include <iostream>
#include <array>
#include <string>
#include "position.h"
#include "globals.h"
#include "attacks.h"
#include "search.h"

// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1

int main(int argc, char** argv)
{
	init_sliding_maps();
	position.board.fill(EMPTY);

	std::string FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1";
	std::string MOVE = "";

	std::string command;
	std::cin >> command;

	if (command == "uci")
	{
		std::cout << "id name Simple\n";
		std::cout << "uciok\n";

		/*
		while (true)
		{
			std::cin >> command;
			if (command == "isready")
			{
				std::cout << "readyok\n";
				continue;
			}
			if (command == "ucinewgame")
			{
				position = Position();
				position.board.fill(EMPTY);
			}
			if (command == "position")
			{
				std::cin >> FEN;
				if (FEN == "startpos")
					FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1";
				position.init(FEN, MOVE);
			}
		}
		*/
	}
	return 0;
}