#include "Quiescence.h"

#include <algorithm>

#include "ExitCondition.h"
#include "MovePicker.h"

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

  if (IsTimeToExit()) {
    return std::nullopt;
  }

  if (current_position.DetectRepetition(current_depth)) {
    return kDrawValue;
  }

  if (current_position.GetHalfMoveClock() >= 100 &&
      (!current_position.IsUnderCheck() ||
       move_generator_.HasLegalMove(current_position))) {
    return kDrawValue;
  }

  if (current_position.IsInsufficientMaterial()) {
    return kDrawValue;
  }

  if (current_depth >= kMaxDepth) {
    return current_position.Evaluate();
  }

  if (current_position.IsUnderCheck()) {
    return SearchUnderCheck(current_position, alpha, beta, current_depth);
  }

  MovePicker picker;
  picker.InitPicker(current_position, nullptr, current_depth);
  auto move = picker.SelectNextMove();
  if (!move.IsValid() && !move_generator_.HasLegalMove(current_position)) {
    return kDrawValue;
  }

  const auto stand_pat = current_position.Evaluate();

  if (stand_pat >= beta) {
    return beta;
  }

  if (alpha < stand_pat) {
    alpha = stand_pat;
  }

  for (; move.IsValid(); move = picker.SelectNextMove()) {
    if (!current_position.StaticExchangeEvaluation(
            move, std::max(1, alpha - stand_pat - kSEEMargin))) {
      continue;
    }

    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval_optional =
        Search<false>(current_position, -beta, -alpha, current_depth + 1);

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (!temp_eval_optional) return std::nullopt;

    const auto temp_eval = -*temp_eval_optional;

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
  MovePicker picker;
  picker.InitPicker(current_position, nullptr, current_depth);
  auto move = picker.SelectNextMove();
  if (!move.IsValid()) {
    return kMateValue +
           static_cast<Eval>(std::min<size_t>(current_depth, kMaxSearchPly));
  }

  for (; move.IsValid(); move = picker.SelectNextMove()) {
    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval_optional =
        Search<false>(current_position, -beta, -alpha, current_depth + 1);

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (!temp_eval_optional) return std::nullopt;

    const auto temp_eval = -*temp_eval_optional;

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
  return exit_condition_.IsTimeToExit();
}
}  // namespace SimpleChessEngine
