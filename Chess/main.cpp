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
	PVmoves.reserve((int)5e7);
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

		auto bm = position.findBestMove(7);
		out << bm.toStr();
		position.doMove(bm);
		out.close();
		std::cout << "End!";
		return 0;
	}

	std::string FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 1";
	//				  "rnb1kb1N/p1pqp1pp/1p3n2/8/2B5/2P1P3/PPP2PPP/R1BQK2R b -- 2"

	std::string MOVE = "";
	position.init(FEN, MOVE);

	while(true)
	{
		counter = 0;
		qcounter = 0;
		auto bm = position.findBestMove(8);
		std::cout << bm.toStr() << " | nodes: " << std::scientific << (double)counter << " q nodes: " << (double)qcounter << std::endl;
		position.doMove(bm);
	}

	return 0;

}