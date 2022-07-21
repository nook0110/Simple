#include <iostream>
#include <array>
#include <string>
#include "position.h"
#include "globals.h"


// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1

int main(int argc, char** argv)
{
	/*
	if (argc > 2)
	{
		std::cout << "Start!";
		std::ifstream in(argv[1]);
		std::string FEN;
		std::string MOVE;
		std::getline(in, FEN);
		std::getline(in, MOVE);
		position.init(FEN, MOVE);
		std::ofstream out(argv[2]);

		auto bm = position.findBestMove();
		out << bm.toStr();
		position.doMove(bm);
		out.close();
		std::cout << "End!";
		return 0;
	}
	*/

	std::string FEN = "1n2K3/5B2/8/1R1RB3/brk1p3/1p1Nr3/3P4/2b5 w -- 1";
	std::string MOVE;
	position.init(FEN, MOVE);

	auto bm = position.findBestMove();

	std::cout << bm.toStr();
	return 0;

}