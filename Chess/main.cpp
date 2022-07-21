#include <iostream>
#include <array>
#include <string>
#include "position.h"
#include "globals.h"


// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1

int main(int argc, char** argv)
{
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

	std::string FEN = "r3k2r/5p1p/1p3p2/4b3/1p1p1P2/1B1K4/PPP3PP/R3R3 b -- 2";
	std::string MOVE;
	position.init(FEN, MOVE);

	auto bm = position.findBestMove();

	std::cout << bm.toStr();
	return 0;

}