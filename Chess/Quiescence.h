#pragma once
#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Position.h"

namespace SimpleChessEngine
{
class Quiescence
{
 public:
  /**
   * \brief Constructor.
   *
   */
  Quiescence() = default;

  /**
   * \brief Performs the alpha-beta search algorithm.
   *
   * \param current_position Current position.
   * \param alpha The current alpha value.
   * \param beta The current beta value.
   *
   * \return Evaluation of subtree.
   */
  template <bool start_of_search>
  [[nodiscard]] Eval Search(Position& current_position, Eval alpha, Eval beta);

  [[nodiscard]] std::size_t GetSearchedNodes() const { return searched_nodes_; }

 private:
  MoveGenerator move_generator_;  //!< Move generator.

  std::size_t searched_nodes_{};
};

template <bool start_of_search>
Eval Quiescence::Search(Position& current_position, Eval alpha, const Eval beta)
{
  if constexpr (start_of_search)
  {
    searched_nodes_ = 0;
  }
  searched_nodes_++;

  const auto stand_pat = current_position.Evaluate();

  if (stand_pat >= beta)
  {
    return stand_pat;
  }
  if (alpha < stand_pat)
  {
    alpha = stand_pat;
  }

  // get all the attacks moves

  for (const auto moves =
           move_generator_.GenerateMoves<MoveGenerator::Type::kQuiescence>(
               current_position);
       const auto& move : moves)
  {
    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval = -Search<false>(current_position, -beta, -alpha);

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (temp_eval > alpha)
    {
      if (temp_eval >= beta)
      {
        return temp_eval;
      }

      alpha = temp_eval;
    }
  }

  return alpha;
}
}  // namespace SimpleChessEngine