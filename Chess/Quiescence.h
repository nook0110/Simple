#pragma once
#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Position.h"
#include "TranspositionTable.h"

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
  bool CompareMoves(const Move& lhs, const Move& rhs, const Position& current_position) const {
    if (lhs.index() != rhs.index()) return lhs.index() < rhs.index();
    if (!std::holds_alternative<DefaultMove>(lhs)) return false;
    const auto lhs_ptr = std::get_if<DefaultMove>(&lhs);
    const auto rhs_ptr = std::get_if<DefaultMove>(&rhs);
    return kPieceValues[static_cast<size_t>(
                            current_position.GetPiece(lhs_ptr->to))]
                   .eval[0] -
               kPieceValues[static_cast<size_t>(
                                current_position.GetPiece(lhs_ptr->from))]
                   .eval[0] <
           kPieceValues[static_cast<size_t>(
                            current_position.GetPiece(rhs_ptr->to))]
                   .eval[0] -
               kPieceValues[static_cast<size_t>(
                                current_position.GetPiece(rhs_ptr->from))]
                   .eval[0];
  }
  
  static constexpr Eval kSmallDelta =
      2 * kPieceValues[static_cast<size_t>(Piece::kPawn)].eval[1];
  static constexpr Eval kBigDelta =
      kPieceValues[static_cast<size_t>(Piece::kQueen)].eval[1];

  TranspositionTable<1 << 24> best_moves_;

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

  if (stand_pat + kBigDelta < alpha) {
    return alpha;
  }

  if (alpha < stand_pat)
  {
    alpha = stand_pat;
  }

  // get all the attacks moves
  auto moves =
      move_generator_.GenerateMoves<MoveGenerator::Type::kQuiescence>(
          current_position);

  std::ranges::stable_sort(moves, [this, &current_position](const Move& lhs, const Move& rhs) {
    return CompareMoves(rhs, lhs, current_position);
  });

  if (best_moves_.Contains(current_position)) {
    const auto tt_move = best_moves_[current_position].move;
    if (auto it = std::find(moves.begin(), moves.end(), tt_move); it != moves.end()) {
      std::iter_swap(it, moves.begin());
    }
  }

  for (const auto& move : moves) {
    if (std::holds_alternative<DefaultMove>(move) &&
        stand_pat +
                kPieceValues[static_cast<size_t>(std::get_if<DefaultMove>(&move)
                                                     ->captured_piece)]
                    .eval[1] +
                kSmallDelta <
            alpha)
      continue;
      
    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval = -Search<false>(current_position, -beta, -alpha);

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (temp_eval > alpha)
    {
      best_moves_[current_position] = {move, current_position.GetHash()};

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