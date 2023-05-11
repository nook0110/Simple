#pragma once
#include <chrono>

#include "Evaluator.h"
#include "Move.h"
#include "Searcher.h"

namespace SimpleChessEngine
{
/**
 * \brief Class that represents a chess engine.
 *
 * \author nook0110
 */
class ChessEngine
{
 public:
  Move ComputeBestMove(const size_t depth);

  Move ComputeBestMove(const std::chrono::milliseconds left_time);

 private:
  Searcher searcher_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine
{
inline Move ChessEngine::ComputeBestMove(const size_t depth)
{
  auto alpha = std::numeric_limits<Eval>::min();
  auto beta = std::numeric_limits<Eval>::max();

  for (size_t current_depth = 0; current_depth < depth;)
  {
    constexpr auto window_size = 10;
    const auto eval = searcher_.Search(current_depth, alpha, beta);

    // check if true eval is out of window
    if (eval <= alpha)
    {
      // search again with a wider window
      alpha -= window_size;

      continue;
    }

    // check if true eval is out of window
    if (eval >= beta)
    {
      // search again with a wider window
      beta += window_size;
      continue;
    }

    // set the window
    alpha = eval - window_size;
    beta = eval + window_size;

    // increase the depth
    current_depth++;
  }

  return searcher_.GetCurrentBestMove();
}

inline Move ChessEngine::ComputeBestMove(
    const std::chrono::milliseconds left_time)
{
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto time_for_move = left_time / 2;
  constexpr size_t max_last_best_move_change = 5;

  auto alpha = std::numeric_limits<Eval>::min();
  auto beta = std::numeric_limits<Eval>::max();

  Move previous_best_move{};
  size_t last_best_move_change{};
  for (size_t current_depth = 0;
       std::chrono::high_resolution_clock::now() - start_time <
           time_for_move  // check if we have time for another iteration
       &&
       last_best_move_change < max_last_best_move_change  // check if best move
                                                          // changed recently
       ;)
  {
    constexpr auto window_size = 10;
    const auto eval = searcher_.Search(current_depth, alpha, beta);

    // check if true eval is out of window
    if (eval <= alpha)
    {
      // search again with a wider window
      alpha -= window_size;

      continue;
    }

    // check if true eval is out of window
    if (eval >= beta)
    {
      // search again with a wider window
      beta += window_size;
      continue;
    }

    // set the window
    alpha = eval - window_size;
    beta = eval + window_size;

    // increase the depth
    current_depth++;

    // check if best move changed
    if (previous_best_move == searcher_.GetCurrentBestMove())
    {
      // increase last change
      ++last_best_move_change;
    }
    else
    {
      // reset last change
      last_best_move_change = 0;
    }

    previous_best_move = searcher_.GetCurrentBestMove();
  }

  return searcher_.GetCurrentBestMove();
}
}  // namespace SimpleChessEngine
