#include <iostream>
#include <array>
#include <string>
#include "position.h"
#include "globals.h"
#include "attacks.h"


// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1

int main(int argc, char** argv)
{
	init_sliding_maps();

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

	std::string FEN = "r1bqk2r/pppp1pp1/2n2n1p/4p3/1b2N3/2N1P3/PPPP1PPP/R1BQKB1R w -- 1";
	//				  "r1bqkb1r/ppp1pppp/2n2n2/3p4/4P3/2N2N2/PPPP1PPP/R1BQKB1R"
	std::string MOVE;
	position.init(FEN, MOVE);

	auto bm = position.findBestMove();

	std::cout << bm.toStr();

	return 0;

}