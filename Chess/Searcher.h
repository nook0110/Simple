#pragma once
#include <algorithm>
#include <optional>
#include <type_traits>

#include "Evaluator.h"
#include "MoveGenerator.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine
{
/**
 * \brief A simple implementation of the alpha-beta search algorithm.
 *
 * \details
 * A list of features:
 *  - PVS
 *  - ZWS
 *  - Fail-soft
 *  - Aspirational windows
 *  - Transposition table
 *  - Iterative deepening
 *  - Quiescence search
 *
 *  \author nook0110
 */
class Searcher
{
 public:
  /**
   * \brief Constructor.
   *
   * \param move_generator The move generator.
   * \param evaluator The evaluator.
   * \param position The initial position.
   */
  explicit Searcher(Position position = Position(),
                    MoveGenerator move_generator = MoveGenerator(),
                    Evaluator evaluator = Evaluator())
      : current_position_(std::move(position)),
        move_generator_(std::move(move_generator)),
        evaluator_(std::move(evaluator))
  {}

  /**
   * \brief Sets the current position.
   *
   * \param position New position.
   */
  void SetPosition(Position position);

  /**
   * \brief Returns the current position.
   *
   * \return The current position.
   */
  [[nodiscard]] const Position& GetPosition() const;

  /**
   * \brief Returns the best move for the current position.
   *
   * \return The current best move.
   */
  [[nodiscard]] Move GetCurrentBestMove() const;

  /**
   * \brief Calculates the best move for the current position.
   *
   * \details This function uses iterative deepening to find the best move.
   *
   * \param depth The maximum depth to search.
   *
   * \return The best move.
   */
  Move ComputeBestMove(const size_t depth);

  /**
   * \brief Performs the alpha-beta search algorithm.
   *
   * \param remaining_depth The remaining depth.
   * \param alpha The current alpha value.
   * \param beta The current beta value.
   *
   * \return Evaluation of subtree.
   */
  [[nodiscard]] Eval Search(const size_t remaining_depth, Eval alpha,
                            const Eval beta);

 private:
  Position current_position_;  //!< Current position.

  MoveGenerator move_generator_;  //!< Move generator.
  Evaluator evaluator_;           //!< Evaluator.

  TranspositionTable
      best_moves_;  //!< Transposition-table to store the best moves.
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine
{
inline void Searcher::SetPosition(Position position)
{
  current_position_ = std::move(position);
}

inline const Position& Searcher::GetPosition() const
{
  return current_position_;
}

inline Move Searcher::GetCurrentBestMove() const
{
  return best_moves_[current_position_];
}

inline Eval Searcher::Search(const size_t remaining_depth, Eval alpha,
                             const Eval beta)
{
  // return the evaluation of the current position if we have reached the end of
  // the search tree
  if (remaining_depth <= 0)
  {
    return evaluator_(current_position_, alpha, beta);
  }

  // lambda function to analyze a move
  auto analyze_move = [this, remaining_depth](const auto& move,
                                              const auto analyzed_alpha,
                                              const auto analyzed_beta)
  {
    // make the move and search the tree
    current_position_.DoMove(move);
    auto temp_eval =
        -Search(remaining_depth - 1, -analyzed_beta, -analyzed_alpha);

    // undo the move
    current_position_.UndoMove(move);

    // return the evaluation
    return temp_eval;
  };

  // get all the possible moves
  auto moves = move_generator_(current_position_);

  // check if there are no possible moves
  if (moves.empty())
  {
    return evaluator_.GetGameResult(current_position_);
  }

  // check if we have already searched this position
  if (best_moves_.Contains(current_position_))
  {
    const auto& best_move = best_moves_[current_position_];

    // find the best move in moves
    // std::iter_swap(std::find(moves.begin(), moves.end(), best_move),
    //               moves.begin());

    // sort all moves except first (PV-move)
    // std::stable_sort(std::next(moves.begin()), moves.end());
  }
  else
  {
    // std::stable_sort(moves.begin(), moves.end());
  }

  const auto& first_move = moves.front();

  auto best_eval = analyze_move(first_move, -beta, -alpha);

  if (best_eval > alpha)
  {
    if (best_eval >= beta)
    {
      return best_eval;
    }

    alpha = best_eval;
  }

  // set best move
  best_moves_[current_position_] = std::move(moves.front());

  // delete first move
  std::iter_swap(moves.begin(), std::prev(moves.end()));
  moves.pop_back();

  // search the tree
  for (auto& move : moves)
  {
    // make the move and search the tree
    current_position_.DoMove(move);

    auto temp_eval = -Search(remaining_depth - 1, -alpha - 1, -alpha);

    // update the best move
    if (temp_eval > alpha && temp_eval < beta)
    {
      temp_eval = -Search(remaining_depth - 1, -alpha, -beta);

      if (temp_eval > alpha)
      {
        alpha = temp_eval;
      }
    }

    // undo the move
    current_position_.UndoMove(move);

    if (temp_eval > best_eval)
    {
      // check if we have found a better move
      if (temp_eval >= beta)
      {
        best_moves_[current_position_] = std::move(move);

        return beta;
      }

      best_moves_[current_position_] = std::move(move);

      best_eval = temp_eval;
    }
  }

  // return the best evaluation
  return best_eval;
}
}  // namespace SimpleChessEngine
