#include "Quiescence.h"

#include <algorithm>

#include "ExitCondition.h"
#include "MovePicker.h"
#include "Nnue/Nnue.h"
#include "Settings.h"

namespace SimpleChessEngine {
template class Quiescence<TimeCondition>;
template class Quiescence<DepthCondition>;
template class Quiescence<Pondering>;

template SearchResult Quiescence<TimeCondition>::Search<false>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth, bool pv_node);
template SearchResult Quiescence<TimeCondition>::Search<true>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth, bool pv_node);
template SearchResult Quiescence<DepthCondition>::Search<false>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth, bool pv_node);
template SearchResult Quiescence<DepthCondition>::Search<true>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth, bool pv_node);
template SearchResult Quiescence<Pondering>::Search<false>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth, bool pv_node);
template SearchResult Quiescence<Pondering>::Search<true>(
    Position& current_position, Eval alpha, const Eval beta,
    const Depth current_depth, bool pv_node);

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
Quiescence<ExitCondition>::Quiescence(const ExitCondition& exit_condition,
                                      Table* table, Age age)
    : exit_condition_(exit_condition), table_(table), age_(age),
      cache_static_eval_(table && Nnue::IsLoaded()),
      eval_context_((static_cast<std::uint64_t>(Nnue::Generation()) << 32) |
                    static_cast<std::uint32_t>(
                        Settings::EvaluationParameters::nnue_scale)) {}

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool start_of_search>
SearchResult Quiescence<ExitCondition>::Search(Position& current_position,
                                               Eval alpha, const Eval beta,
                                               const Depth current_depth,
                                               const bool pv_node) {
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

  const auto tt_node = table_ ? table_->GetNode(current_position) : Node{};
  const auto tt_value = FromTTScore(tt_node.score, current_depth);
  if (!pv_node && tt_node.IsOccupied() && tt_node.depth >= kTTDepth &&
      (tt_node.bound & (tt_value >= beta ? Bound::kLower : Bound::kUpper))) {
    return tt_value;
  }

  Move best_move = Move::None();
  if (current_position.IsUnderCheck()) {
    const auto value =
        SearchUnderCheck(current_position, alpha, beta, current_depth, pv_node,
                         tt_node.move, best_move);
    if (value) {
      Store(current_position, best_move, *value,
            *value >= beta ? Bound::kLower : Bound::kUpper, current_depth,
            kNoStaticEval);
    }
    return value;
  }

  const auto tt_move =
      tt_node.move.IsValid() && !tt_node.move.IsCastling() &&
              (tt_node.move.IsEnPassant() ||
               current_position.GetPieceAt(tt_node.move.To()) != Piece::kNone)
          ? tt_node.move
          : Move::None();
  MovePicker picker;
  picker.InitPicker(current_position, nullptr, current_depth, tt_move);
  auto move = picker.SelectNextMove();
  if (!move.IsValid() && !move_generator_.HasLegalMove(current_position)) {
    return kDrawValue;
  }

  Eval static_eval = cache_static_eval_
                         ? table_->GetStaticEval(tt_node, eval_context_)
                         : kNoStaticEval;
  if (static_eval == kNoStaticEval) static_eval = current_position.Evaluate();
  Eval best_value = static_eval;
  if (tt_node.IsOccupied() &&
      std::abs(tt_value) < kTablebaseWinValue - kMaxSearchPly &&
      (tt_node.bound &
       (tt_value > best_value ? Bound::kLower : Bound::kUpper))) {
    best_value = tt_value;
  }
  const auto stand_pat = best_value;

  if (best_value >= beta) {
    if (!tt_node.IsOccupied()) {
      Store(current_position, best_move, best_value, Bound::kLower,
            current_depth, static_eval);
    }
    return best_value;
  }

  alpha = std::max(alpha, best_value);

  for (; move.IsValid(); move = picker.SelectNextMove()) {
    if (!current_position.StaticExchangeEvaluation(
            move, std::max(1, alpha - stand_pat - kSEEMargin))) {
      continue;
    }

    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval_optional = Search<false>(
        current_position, -beta, -alpha, current_depth + 1, pv_node);

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (!temp_eval_optional) return std::nullopt;

    const auto temp_eval = -*temp_eval_optional;

    if (temp_eval > best_value) {
      best_value = temp_eval;
      if (temp_eval > alpha) {
        best_move = move;
        if (temp_eval >= beta) break;
        alpha = temp_eval;
      }
    }
  }

  Store(current_position, best_move, best_value,
        best_value >= beta ? Bound::kLower : Bound::kUpper, current_depth,
        static_eval);
  return best_value;
}

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult Quiescence<ExitCondition>::SearchUnderCheck(
    Position& current_position, Eval alpha, Eval beta,
    const Depth current_depth, const bool pv_node, const Move tt_move,
    Move& best_move) {
  MovePicker picker;
  picker.InitPicker(current_position, nullptr, current_depth, tt_move);
  auto move = picker.SelectNextMove();
  if (!move.IsValid()) {
    return kMateValue +
           static_cast<Eval>(std::min<size_t>(current_depth, kMaxSearchPly));
  }

  Eval best_value = kMateValue;
  for (; move.IsValid(); move = picker.SelectNextMove()) {
    const auto irreversible_data = current_position.GetIrreversibleData();

    // make the move and search the tree
    current_position.DoMove(move);
    const auto temp_eval_optional = Search<false>(
        current_position, -beta, -alpha, current_depth + 1, pv_node);

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (!temp_eval_optional) return std::nullopt;

    const auto temp_eval = -*temp_eval_optional;

    if (temp_eval > best_value) {
      best_value = temp_eval;
      if (temp_eval > alpha) {
        best_move = move;
        if (temp_eval >= beta) break;
        alpha = temp_eval;
      }
    }
  }

  return best_value;
}

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
void Quiescence<ExitCondition>::Store(const Position& position, const Move move,
                                      const Eval value, const Bound bound,
                                      const Depth ply, const Eval static_eval) {
  if (!table_) return;
  table_->SetEntry(position, move, ToTTScore(value, ply), kTTDepth, bound, age_,
                   cache_static_eval_ ? static_eval : kNoStaticEval,
                   eval_context_);
}

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool Quiescence<ExitCondition>::IsTimeToExit() {
  return exit_condition_.IsTimeToExit();
}
}  // namespace SimpleChessEngine
