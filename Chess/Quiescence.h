#pragma once
#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Position.h"

namespace SimpleChessEngine {
class Quiescence {
 public:
  constexpr static size_t kEnoughNodesToCheckTime = 1 << 12;

  /**
   * \brief Constructor.
   *
   */
  Quiescence(const TimePoint& end_time) : end_time_(end_time){};

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
  [[nodiscard]] SearchResult Search(Position& current_position, Eval alpha,
                                    Eval beta);

  [[nodiscard]] std::size_t GetSearchedNodes() const { return searched_nodes_; }

 private:
  bool CompareMoves(const Move& lhs, const Move& rhs,
                    const Position& current_position) const {
    if (lhs.index() != rhs.index()) return lhs.index() > rhs.index();
    if (!std::holds_alternative<DefaultMove>(lhs)) return false;
    const auto [from_lhs, to_lhs, captured_piece_lhs] = GetMoveData(lhs);
    const auto [from_rhs, to_rhs, captured_piece_rhs] = GetMoveData(rhs);
    const auto captured_idx_lhs = static_cast<int>(captured_piece_lhs);
    const auto captured_idx_rhs = static_cast<int>(captured_piece_rhs);
    const auto moving_idx_lhs =
        -static_cast<int>(current_position.GetPiece(from_lhs));
    const auto moving_idx_rhs =
        -static_cast<int>(current_position.GetPiece(from_rhs));
    return std::tie(captured_idx_lhs, moving_idx_lhs) >
           std::tie(captured_idx_rhs, moving_idx_rhs);
  }

  bool IsTimeToExit() {
    return searched_nodes_ % kEnoughNodesToCheckTime == 0 &&
           std::chrono::high_resolution_clock::now() > end_time_;
  }

  static constexpr Eval kSmallDelta =
      2 * kPieceValues[static_cast<size_t>(Piece::kPawn)].eval[1];
  static constexpr Eval kBigDelta =
      kPieceValues[static_cast<size_t>(Piece::kQueen)].eval[1];

  MoveGenerator move_generator_;  //!< Move generator.

  const TimePoint& end_time_;

  std::size_t searched_nodes_{};
};

template <bool start_of_search>
SearchResult Quiescence::Search(Position& current_position, Eval alpha,
                                const Eval beta) {
  if constexpr (start_of_search) {
    searched_nodes_ = 0;
  }
  searched_nodes_++;

  const auto stand_pat = current_position.Evaluate();

  if (stand_pat >= beta) {
    return stand_pat;
  }

  if (stand_pat + kBigDelta < alpha) {
    return alpha;
  }

  if (alpha < stand_pat) {
    alpha = stand_pat;
  }

  // get all the attacks moves
  auto moves = move_generator_.GenerateMoves<MoveGenerator::Type::kQuiescence>(
      current_position);

  std::ranges::sort(
      moves, [this, &current_position](const Move& lhs, const Move& rhs) {
        return CompareMoves(lhs, rhs, current_position);
      });

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
    const auto temp_eval_optional =
        Search<false>(current_position, -beta, -alpha);

    if (!temp_eval_optional) return std::nullopt;

    const auto temp_eval = -*temp_eval_optional;

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (temp_eval > alpha) {
      if (temp_eval >= beta) {
        return temp_eval;
      }

      alpha = temp_eval;
    }
  }

  return alpha;
}
}  // namespace SimpleChessEngine