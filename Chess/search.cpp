#include "search.h"
#include <iostream>

extern std::vector<nodeInfo> tp_table(tableSize, nodeInfo());
extern std::unordered_map<key_t, nodeInfo> PVmoves = std::unordered_map<key_t, nodeInfo>();
std::mutex updateBM;
unsigned long long qcounter;

void findMove(Position& pos, const std::vector<Move>& moves, value& alpha, value& beta, Move& bestMove, unsigned char maxDepth)
{
	for (int i = 0; i < moves.size(); ++i)
	{
		const CastlingRight cr_w = pos.cr[COLOR_W];
		const CastlingRight cr_b = pos.cr[COLOR_B];
		pos.doMove(moves[i]);
		auto tempAlpha = pos.findAlphaBeta(1, alpha, beta, moves[i], maxDepth);
		pos.undoMove(moves[i]);
		pos.cr[COLOR_W] = cr_w;
		pos.cr[COLOR_B] = cr_b;
		if (!tempAlpha.has_value())
		{
			continue;
		}
		updateBM.lock();
		if (alpha < tempAlpha.value())
		{
			bestMove = moves[i];
			alpha = tempAlpha.value();
		}
		updateBM.unlock();
		if (alpha >= beta)
			break;
	}
}

Move Position::findBestMove(unsigned char maxDepth, std::vector<Move>& moves)
{
	++counter;

	auto [pv_iterator, inserted] = PVmoves.insert({ hash, nodeInfo() });
	if (maxDepth <= pv_iterator->second.maxDepth && 0 >= pv_iterator->second.depth)
		return  pv_iterator->second.bestMove;

	value alpha = INT_MIN;
	value beta = INT_MAX;

	if (!inserted)
	{
		auto pvInMoves = std::find(moves.begin(), moves.end(), pv_iterator->second.bestMove);
		std::iter_swap(moves.begin(), pvInMoves);
		alpha = pv_iterator->second.eval - pieceValues[PAWN_W][EG];
		beta = pv_iterator->second.eval + pieceValues[PAWN_W][EG];
	}

	std::optional<value> tempAlpha;

	size_t incorrect = 0;
	size_t cuts = 0;

	for (const auto& move : moves)
	{
		doMove(move);
		tempAlpha = findAlphaBeta(1, alpha, beta, move, maxDepth);
		undoMove(move);
		if (!tempAlpha.has_value())
		{
			++cuts;
			continue;
		}
		if (alpha < tempAlpha.value())
		{
			pv_iterator->second.bestMove = move;
			pv_iterator->second.depth = 0;
			pv_iterator->second.maxDepth = maxDepth;
			alpha = tempAlpha.value();
			pv_iterator->second.eval = alpha;
		}
	}

	return pv_iterator->second.bestMove;
}

Move Position::findBestMove(unsigned char minDepth, unsigned char maxDepth)
{
	auto moves = generateMoves();
	Move bestMove = moves[0];
	value alpha = INT_MIN;
	value beta = INT_MAX;
	Position c1 = *this, c2 = *this, c3 = *this, c4 = *this, c5 = *this, c6 = *this, c7 = *this;
	auto shift = moves.size() / 8;
	auto remainder = moves.size() % 8;
	std::array<std::vector<Move>::iterator, 9> its;

	its[0] = moves.begin();
	for (unsigned long long i = 1; i < 8; ++i)
	{
		its[i] = moves.begin() + shift * i;
		its[i] += std::min(i, remainder);
	}
	its[8] = moves.end();
	for (int i = 0; i < 8; ++i)
	{
		std::sort(its[i], its[i + 1]);
	}

	for (int depth = 4; depth < maxDepth; ++depth)
	{
		alpha = INT_MIN;
		beta = INT_MAX;
#pragma omp parallel sections num_threads(1)
		{
#pragma omp section
			{
				findMove(*this, std::vector<Move>(its[0], its[1]), alpha, beta, bestMove, depth);
			}
#pragma omp section
			{
				findMove(c1, std::vector<Move>(its[1], its[2]), alpha, beta, bestMove, depth);
			}
#pragma omp section
			{
				findMove(c2, std::vector<Move>(its[2], its[3]), alpha, beta, bestMove, depth);
			}
#pragma omp section
			{
				findMove(c3, std::vector<Move>(its[3], its[4]), alpha, beta, bestMove, depth);
			}
#pragma omp section
			{
				findMove(c4, std::vector<Move>(its[4], its[5]), alpha, beta, bestMove, depth);
			}
#pragma omp section
			{
				findMove(c5, std::vector<Move>(its[5], its[6]), alpha, beta, bestMove, depth);
			}
#pragma omp section
			{
				findMove(c6, std::vector<Move>(its[6], its[7]), alpha, beta, bestMove, depth);
			}
#pragma omp section
			{
				findMove(c7, std::vector<Move>(its[7], its[8]), alpha, beta, bestMove, depth);
			}
		}
		std::cout << "Depth " << depth << ": " << bestMove.toStr() << std::endl;
	}
	return bestMove;
}

std::optional<value> Position::findAlphaBeta(int depth, value alpha, value beta, const Move& previous, const unsigned char maxDepth)
{
	++counter;

	if (depth > maxDepth)
	{
		auto eval = quiesce(depth, alpha, beta);
		return eval;
	/*
	auto pv_iterator = PVmoves.insert({ hash, nodeInfo() }).first;
	if (maxDepth <= pv_iterator->second.maxDepth && depth >= pv_iterator->second.depth)
	*/
		return  pv_iterator->second.eval;

	auto moves = generateMoves();
	/*
	if (pv_iterator->second.depth != 255)
	if (pv_iterator->second.depth != 255)
	{
		auto pvInMoves = std::find(moves.begin(), moves.end(), pv_iterator->second.bestMove);
		std::iter_swap(moves.begin(), pvInMoves);
		std::sort(moves.begin() + 1, moves.end());
		alpha = pv_iterator->second.eval - pieceValues[PAWN_W][EG];
		beta = pv_iterator->second.eval + pieceValues[PAWN_W][EG];
	}
	else
	*/
	{
	//}
	}
	if (depth % 2 == 0)
	{
		std::optional<value> tempAlpha;

		for (const auto& move : moves)
		{
			const CastlingRight cr_w = cr[COLOR_W];
			const CastlingRight cr_b = cr[COLOR_B];
			doMove(move);
			tempAlpha = findAlphaBeta(depth + 1, alpha, beta, move, maxDepth);
			undoMove(move);
			cr[COLOR_W] = cr_w;
			cr[COLOR_B] = cr_b;
			if (!tempAlpha.has_value())
			{
				++cuts;
				continue;
			}
			if (alpha < tempAlpha.value())
			{
				alpha = tempAlpha.value();
			}
			if (beta <= alpha)
			{
				return beta;
			}
		}

		if (cuts == moves.size())
		{

		}

		return alpha;
	}
	else
	{
		std::optional<value> tempBeta;

		for (const auto& move : moves)
		{
			const CastlingRight cr_w = cr[COLOR_W];
			const CastlingRight cr_b = cr[COLOR_B];
			doMove(move);
			tempBeta = findAlphaBeta(depth + 1, alpha, beta, move, maxDepth);
			undoMove(move);
			cr[COLOR_W] = cr_w;
			cr[COLOR_B] = cr_b;
			if (!tempBeta.has_value())
			{
				++cuts;
				continue;
			}
			if (beta > tempBeta.value())
			{
				beta = tempBeta.value();
			}
			if (alpha >= beta)
			{
				return alpha;
			}
		}

		return beta;
	}

}

std::optional<value> Position::quiesce(int depth, value alpha, value beta)
{
	++qcounter;

	value standingPat = (sideToMove == (depth % 2) ? -evaluate() : evaluate());

	auto us = static_cast<Color>(sideToMove);
	auto them = flip(us);

	if (underCheck(them))
	{
		return std::nullopt;
	}

	size_t incorrect = 0;
	auto moves = generateAttacks();

	if (depth % 2 == 0)
	{
		if (standingPat >= beta)
			return standingPat;
		if (alpha < standingPat)
			alpha = standingPat;
		std::optional<value> tempAlpha;
		std::sort(moves.begin(), moves.end());

		for (const auto& move : moves)
		{
			const CastlingRight cr_w = cr[COLOR_W];
			const CastlingRight cr_b = cr[COLOR_B];
			doMove(move);
			tempAlpha = quiesce(depth + 1, alpha, beta);
			undoMove(move);
			cr[COLOR_W] = cr_w;
			cr[COLOR_B] = cr_b;
			if (!tempAlpha.has_value())
			{
				++incorrect;
				continue;
			}
			if (beta <= tempAlpha.value())
			{
				return beta;
			}
			if (alpha < tempAlpha.value())
			{
				alpha = tempAlpha.value();
			}
		}

		return alpha;
	}
	else
	{
		if (standingPat <= alpha)
			return standingPat;
		if (beta > standingPat)
			beta = standingPat;
		std::optional<value> tempBeta;
		std::sort(moves.begin(), moves.end());

		for (const auto& move : moves)
		{
			const CastlingRight cr_w = cr[COLOR_W];
			const CastlingRight cr_b = cr[COLOR_B];
			doMove(move);
			tempBeta = quiesce(depth + 1, alpha, beta);
			undoMove(move);
			cr[COLOR_W] = cr_w;
			cr[COLOR_B] = cr_b;
			if (!tempBeta.has_value())
			{
				++incorrect;
				continue;
			}
			if (alpha >= tempBeta.value())
			{
				return alpha;
			}
			if (beta > tempBeta.value())
			{
				beta = tempBeta.value();
			}
		}

		return beta;
	}
}