#include "search.h"

extern std::vector<nodeInfo> tp_table(tableSize, nodeInfo());

std::mutex updateBM;

void findMove(Position& pos, const std::vector<Move>& moves, value& alpha, value& beta, Move& bestMove)
{
	for (int i = 0; i < moves.size(); ++i)
	{
		pos.doMove(moves[i]);
		auto tempAlpha = pos.findAlphaBeta(1, alpha, beta, moves[i]);
		pos.undoMove(moves[i]);
		if (!tempAlpha.has_value())
		{
			continue;
		}
		if (alpha < tempAlpha.value())
		{
			updateBM.lock();
			bestMove = moves[i];
			alpha = tempAlpha.value();
			updateBM.unlock();
		}
		if (alpha >= beta)
			break;
	}
}

Move Position::findBestMove()
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

#pragma omp parallel sections num_threads(8)
	{
#pragma omp section
		{
			findMove(*this, std::vector<Move>(its[0], its[1]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c1, std::vector<Move>(its[1], its[2]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c2, std::vector<Move>(its[2], its[3]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c3, std::vector<Move>(its[3], its[4]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c4, std::vector<Move>(its[4], its[5]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c5, std::vector<Move>(its[5], its[6]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c6, std::vector<Move>(its[6], its[7]), alpha, beta, bestMove);
		}
#pragma omp section
		{
			findMove(c7, std::vector<Move>(its[7], its[8]), alpha, beta, bestMove);
		}
	}

	return bestMove;
}

std::optional<value> Position::findAlphaBeta(int depth, value alpha, value beta, const Move& previous)
{
	auto us = static_cast<Color>(sideToMove);
	auto them = flip(us);
	if (underCheck(them))
	{
		return std::nullopt;
	}
	if (((depth > 5 && previous.captured == '-') && !underCheck(us)) || depth > 8)
	{
		return (sideToMove == (depth % 2) ? -evaluate() : evaluate());
	}
	size_t incorrect = 0;
	if (depth % 2 == 0)
	{
		std::optional<value> tempAlpha;
		auto moves = generateMoves();
		std::sort(moves.begin(), moves.end());
		for (const auto& move : moves)
		{
			doMove(move);
			tempAlpha = findAlphaBeta(depth + 1, alpha, beta, move);
			undoMove(move);
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
		if (incorrect == moves.size())
		{
			if ((attackMap[kingPos[sideToMove]] & (color[!sideToMove] ^ pawns(them))).any() || (king(us) & pawnAttacks(them)).any())
			{
				return -1e9 + (depth << 4);
			}
			else
			{
				return 0;
			}
		}
		return alpha;
	}
	else
	{
		std::optional<value> tempBeta;
		auto moves = generateMoves();
		std::sort(moves.begin(), moves.end());
		for (const auto& move : moves)
		{
			doMove(move);
			tempBeta = findAlphaBeta(depth + 1, alpha, beta, move);
			undoMove(move);
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
		if (incorrect == moves.size())
		{
			if ((attackMap[kingPos[sideToMove]] & (color[!sideToMove] ^ pawns(them))).any() || (king(us) & pawnAttacks(them)).any())
			{
				return 1e9 - (depth << 4);
			}
			else
			{
				return 0;
			}
		}
		return beta;
	}

}