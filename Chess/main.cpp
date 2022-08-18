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
	//PVmoves.reserve((int)5e7);
	position.board.fill(EMPTY);

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

		auto bm = position.findBestMove(7,7);
		out << bm.toStr();
		position.doMove(bm);
		out.close();
		std::cout << bm.toStr() << " | nodes: " << std::scientific << (double)counter << " q nodes: " << (double)qcounter << std::endl;
		std::cout << "End!";
		return 0;
	}

	std::string FEN = "3r2k1/p1N2p1r/2pP1P1p/q1P3p1/P2Bp3/2N1R3/5PP1/1R2K3 b -- 1";
	//				  "r1b1kb1r/ppp2ppp/2n2n2/q3p3/2PP4/2NBB3/PP3PPP/R2QK1NR w -- 2"

	std::string MOVE = "";
	position.init(FEN, MOVE);

	//while(true)
	//{
		counter = 0;
		qcounter = 0;
		auto bm = position.findBestMove(6,6);
		std::cout << bm.toStr() << " | nodes: " << std::scientific << (double)counter << " q nodes: " << (double)qcounter << std::endl;
		position.doMove(bm);
	//}

	return 0;

}