#include <iostream>
#include <array>
#include <string>
#include "position.h"
#include "globals.h"


// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1

int main()
{
	std::string FEN;
	std::string MOVE;
	std::getline(std::cin, FEN);
	std::getline(std::cin, MOVE);
	position.init(FEN, MOVE);
	while (true)
	{

		auto bm = position.findBestMove();
		position.doMove(bm);
		//std::cout << position.board << std::endl;
		std::cout << (char)(bm.from.file + 'a') << 8 - bm.from.rank << " " << (char)(bm.to.file + 'a') << 8 - bm.to.rank << " " << std::endl;
		//std::cout << position.evaluate() << std::endl;
		++counter;
	}
	while (true)
	{
		std::cout << position.evaluate() << std::endl;
		std::string move;
		std::getline(std::cin, move);
		if (move == "-")
		{
			break;
		}
		Move _move = { SQUARE(8 - (move[1] - '0'), move[0] - 'a'),
			SQUARE(8 - (move[4] - '0'), move[3] - 'a'),
			static_cast<MoveType>(move[6] - '0'),
			position.board[SQUARE(8 - (move[4] - '0'), move[3] - 'a')]
		};
		position.doMove(_move);
		std::cout << position.board << std::endl;
	}
	position.log();
	return 0;
}