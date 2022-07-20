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
		std::cout << argc << std::endl;
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
		position.log();
		out.close();
		return 0;
	}

	std::ifstream in("Moves\\Imove0.txt");
	std::string FEN;
	std::string MOVE;
	std::getline(in, FEN);
	std::getline(in, MOVE);
	position.init(FEN, MOVE);
	std::ofstream out("Moves\\Omove0.txt");

	//auto bm = position.findBestMove();
	//out << bm.toStr();
	//out.close();
	//position.doMove(bm);
	//position.log();
	//return 0;






	/*
	const Square from(8 - (move[1] - '0'), move[0] - 'a');
	const Square to(8 - (move[4] - '0'), move[3] - 'a');
	if (nonPawn(board[from.getInd()]))
		return DEFAULT;
	if (from.file == to.file)
		return DEFAULT;
	if (to.rank == 0 || to.rank == 7)
		return PROMOTION;
	return (board[to.getInd()] == '-' ? EN_PASSANT : DEFAULT);
	std::string move = "e2 e4 0";
	Move _move = { SQUARE(8 - (move[1] - '0'), move[0] - 'a'),
			SQUARE(8 - (move[4] - '0'), move[3] - 'a'),
			static_cast<MoveType>(move[6] - '0'),
			position.board[SQUARE(8 - (move[4] - '0'), move[3] - 'a')] };
	position.doMove(_move);
	move = "e7 e5 0";
	_move = { SQUARE(8 - (move[1] - '0'), move[0] - 'a'),
			SQUARE(8 - (move[4] - '0'), move[3] - 'a'),
			static_cast<MoveType>(move[6] - '0'),
			position.board[SQUARE(8 - (move[4] - '0'), move[3] - 'a')] };
	position.doMove(_move);
	position.undoMove(_move);
	move = "e7 e6 0";
	_move = { SQUARE(8 - (move[1] - '0'), move[0] - 'a'),
			SQUARE(8 - (move[4] - '0'), move[3] - 'a'),
			static_cast<MoveType>(move[6] - '0'),
			position.board[SQUARE(8 - (move[4] - '0'), move[3] - 'a')] };
	position.doMove(_move);
	auto moves = position.generateMoves();
	for (auto& m : moves)
	{
		std::cout << m.toStr() << std::endl;
	}
	return 0;
	*/
	while (true)
	{

		auto bm = position.findBestMove();
		position.doMove(bm);
		//std::cout << position.board << std::endl;
		std::cout << (char)(bm.from.file + 'a') << 8 - bm.from.rank << " " << (char)(bm.to.file + 'a') << 8 - bm.to.rank << " " << std::endl;
		std::cout << position.evaluate() << std::endl;
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