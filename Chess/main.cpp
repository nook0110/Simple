#include <iostream>
#include <array>
#include <string>
#include "position.h"
#include "globals.h"


// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 0 1

int main()
{
	std::string FEN;
	std::getline(std::cin, FEN);
	position.init(FEN);
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
			static_cast<MoveType>(move[6]-'0'),
			position.board[SQUARE(move[3] - 'a' + 1, move[4] - '0')]
		};
		position.doMove(_move);
		std::cout << position.board << std::endl;
	}
	position.logAttackMap();
	return 0;
}