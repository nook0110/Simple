#include "Quiescence.h"

#include "ExitCondition.h"

namespace SimpleChessEngine {
template class Quiescence<TimeCondition>;
template class Quiescence<DepthCondition>;
template class Quiescence<Pondering>;

template SearchResult Quiescence<TimeCondition>::Search<false>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth);
template SearchResult Quiescence<TimeCondition>::Search<true>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth);
template SearchResult Quiescence<DepthCondition>::Search<false>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth);
template SearchResult Quiescence<DepthCondition>::Search<true>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth);
template SearchResult Quiescence<Pondering>::Search<false>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth);
template SearchResult Quiescence<Pondering>::Search<true>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth);

auto kCompareMoves = [](const Move& lhs, const Move& rhs,
                        const Position& current_position) {
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
};

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool start_of_search>
SearchResult Quiescence<ExitCondition>::Search(Position& current_position,
                                               Eval alpha, const Eval beta,
                                               const Depth current_depth) {
  if constexpr (start_of_search) {
    searched_nodes_ = 0;
  }
  searched_nodes_++;

  if (current_position.IsUnderCheck()) {
    return SearchUnderCheck(current_position, alpha, beta, current_depth);
  }

  const auto stand_pat = current_position.Evaluate();

  if (stand_pat >= beta) {
    return beta;
  }

  if (alpha < stand_pat) {
    alpha = stand_pat;
  }

  // get all the attacks moves
  auto moves = move_generator_.GenerateMoves<MoveGenerator::Type::kQuiescence>(
      current_position);

  // std::ranges::sort(moves, kCompareMoves);

  for (const auto& move : moves) {
    if (!current_position.StaticExchangeEvaluation(
            move, std::max(1, alpha - stand_pat - kSEEMargin))) {
      continue;
    }

    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval_optional =
        Search<false>(current_position, -beta, -alpha, current_depth + 1);

    if (!temp_eval_optional) return std::nullopt;

    const auto temp_eval = -*temp_eval_optional;

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (temp_eval > alpha) {
      if (temp_eval >= beta) {
        return beta;
      }

      alpha = temp_eval;
    }
  }

  return alpha;
}

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult Quiescence<ExitCondition>::SearchUnderCheck(
    Position& current_position, Eval alpha, Eval beta,
    const Depth current_depth) {
  MoveGenerator::Moves moves =
      move_generator_.GenerateMoves<MoveGenerator::Type::kAll>(
          current_position);

  if (moves.empty()) {
    return kMateValue + kMaxSearchPly;
  }

  // std::ranges::sort(moves, kCompareMoves);

  for (const auto& move : moves) {
    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval_optional =
        Search<false>(current_position, -beta, -alpha, current_depth + 1);

    if (!temp_eval_optional) return std::nullopt;

    const auto temp_eval = -*temp_eval_optional;

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (temp_eval > alpha) {
      if (temp_eval >= beta) {
        return beta;
      }

      alpha = temp_eval;
    }
  }

  return alpha;
}

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool Quiescence<ExitCondition>::IsTimeToExit() {
  return searched_nodes_ % kEnoughNodesToCheckTime == 0 &&
         exit_condition_.IsTimeToExit();
}
}  // namespace SimpleChessEngine