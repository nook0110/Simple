#pragma once
#include <algorithm>

#include "Evaluation.h"
#include "MoveGenerator.h"
#include "PositionFactory.h"
#include "Quiescence.h"
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
 *  - Aspiration windows
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
   * \param position The initial position.
   */
  explicit Searcher(const Position position = PositionFactory{}())
      : current_position_(position),
        move_generator_(MoveGenerator()),
        quiescence_searcher_()
  {}

  /**
   * \brief Sets the current position.
   *
   * \param position New position.
   */
  void SetPosition(const Position& position);

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
  [[nodiscard]] const Move& GetCurrentBestMove() const;

  [[nodiscard]] const TranspositionTable& GetTranspositionTable() const;

  /**
   * \brief Performs the alpha-beta search algorithm.
   *
   * \param remaining_depth The remaining depth.
   * \param alpha The current alpha value.
   * \param beta The current beta value.
   *
   * \return Evaluation of subtree.
   */
  template <bool start_of_search>
  [[nodiscard]] Eval Search(size_t remaining_depth, Eval alpha, Eval beta);

 private:
  Move best_move_;

  Position current_position_;  //!< Current position.

  MoveGenerator move_generator_;  //!< Move generator.
  Quiescence quiescence_searcher_;

  TranspositionTable
      best_moves_;  //!< Transposition-table to store the best moves.
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine
{
inline void Searcher::SetPosition(const Position& position)
{
  current_position_ = position;
}

inline const Position& Searcher::GetPosition() const
{
  return current_position_;
}

inline const Move& Searcher::GetCurrentBestMove() const { return best_move_; }

inline const TranspositionTable& Searcher::GetTranspositionTable() const
{
  return best_moves_;
}

template <bool start_of_search>
Eval Searcher::Search(const size_t remaining_depth, Eval alpha, const Eval beta)
{
  // return the evaluation of the current position if we have reached the
  // end of the search tree
  if (remaining_depth <= 0)
  {
    return quiescence_searcher_.Search(current_position_, alpha, beta);
  }

  // lambda function to analyze a move
  auto analyze_move = [this, remaining_depth](const auto& move,
                                              const auto analyzed_alpha,
                                              const auto analyzed_beta)
  {
    const auto irreversible_data = current_position_.GetIrreversibleData();

    // make the move and search the tree
    current_position_.DoMove(move);
    auto temp_eval =
        -Search<false>(remaining_depth - 1, -analyzed_beta, -analyzed_alpha);

    // undo the move
    current_position_.UndoMove(move, irreversible_data);

    // return the evaluation
    return temp_eval;
  };
  // lambda function that sets best move
  auto set_best_move = [this](const Move& move)
  {
    best_moves_[current_position_] = {move, current_position_.GetHash()};

    if constexpr (start_of_search)
    {
      best_move_ = move;
    }
  };

  // get all the possible moves
  auto moves = move_generator_.GenerateMoves<MoveGenerator::Type::kDefault>(
      current_position_);

  // check if there are no possible moves
  if (moves.empty())
  {
    if (current_position_.IsUnderCheck())
    {
      return kMateValue - static_cast<Eval>(remaining_depth);
    }
    return Eval{};
  }

  // check if we have already searched this position
  if (best_moves_.Contains(current_position_))
  {
    const auto& best_move = best_moves_[current_position_].move;

    // find the best move in moves

    if (const auto pv = std::ranges::find(moves, best_move); pv != moves.end())
      std::iter_swap(pv, moves.begin());

    // sort all moves except first (PV-move)
    std::sort(std::next(moves.begin()), moves.end(), std::greater{});
  }
  else
  {
    std::ranges::sort(moves, std::greater{});
  }

  const auto& first_move = moves.front();

  auto best_eval = analyze_move(first_move, alpha, beta);

  if (best_eval > alpha)
  {
    if (best_eval >= beta)
    {
      return best_eval;
    }

    alpha = best_eval;
  }

  // set best move
  set_best_move(moves.front());

  // delete first move
  std::iter_swap(moves.begin(), std::prev(moves.end()));
  moves.pop_back();

  // search the tree
  for (const auto& move : moves)
  {
    const auto irreversible_data = current_position_.GetIrreversibleData();

    // make the move and search the tree
    current_position_.DoMove(move);

    // ZWS
    auto temp_eval = -Search<false>(remaining_depth - 1, -alpha - 1, -alpha);

    // make a research (ZWS failed)
    if (temp_eval > alpha && temp_eval < beta)
    {
      temp_eval = -Search<false>(remaining_depth - 1, -beta, -alpha);

      if (temp_eval > alpha)
      {
        alpha = temp_eval;
      }
    }

    // undo the move
    current_position_.UndoMove(move, irreversible_data);

    if (temp_eval > best_eval)
    {
      // check if we have found a better move
      if (temp_eval >= beta)
      {
        set_best_move(move);

        return temp_eval;
      }

      set_best_move(move);

      best_eval = temp_eval;
    }
  }

  // return the best evaluation
  return best_eval;
}
}  // namespace SimpleChessEngine
