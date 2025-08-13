#include "SearchImplementation.h"

#include <cassert>

#include "ExitCondition.h"
#include "MovePicker.h"
#include "Position.h"
#include "Quiescence.h"
#include "Searcher.h"
#include "Settings.h"

namespace SimpleChessEngine {
template struct SearchNode<false, TimeCondition>;
template struct SearchNode<true, TimeCondition>;
template struct SearchNode<false, DepthCondition>;
template struct SearchNode<true, DepthCondition>;
template struct SearchNode<false, Pondering>;
template struct SearchNode<true, Pondering>;

PositionInfo::PositionInfo(const Position &position)
    : static_eval(position.Evaluate()), is_under_check(position.IsUnderCheck()),
      irreversible_data(position.GetIrreversibleData()),
      side_to_move_idx(static_cast<size_t>(position.GetSideToMove())) {}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchNode<is_principal_variation, ExitCondition>::SearchNode(
    Searcher &searcher, SearchState state, const ExitCondition &exit_condition)
    : state_(state), exit_condition_(exit_condition),
      position_info_(searcher.GetPosition()), searcher_(searcher) {}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_principal_variation_search>
SearchResult SearchNode<is_principal_variation, ExitCondition>::StartSubsearch(
    SearchState state) {
  assert(is_principal_variation_search || (state.beta - state.alpha == 1));
  assert(state.alpha < state.beta);

  DLOG(INFO) << "PV: " << std::boolalpha << is_principal_variation_search;

  return SearchNode<is_principal_variation_search, ExitCondition>{
      searcher_, state, exit_condition_}();
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<is_principal_variation, ExitCondition>::IsTimeToExit() const {
  return searcher_.debug_info_.searched_nodes % kEnoughNodesToCheckTime == 0 &&
         exit_condition_.IsTimeToExit();
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SearchNode<is_principal_variation, ExitCondition>::operator()() {
  if (IsTimeToExit()) {
    return std::nullopt;
  }

  auto &[max_depth, remaining_depth, alpha, beta, _] = state_;
  assert(is_principal_variation || beta - alpha == 1);

  if (GetCurrentPosition().DetectRepetition(max_depth - remaining_depth)) {
    return kDrawValue;
  }

  // return the evaluation of the current position if we have reached
  // the end of the search tree
  if (remaining_depth <= 0) {
    return QuiescenceSearch();
  }

  searcher_.debug_info_.searched_nodes++;

  if (!ProbeTranspositionTable()) {
    // state_.remaining_depth--;
  }

  if (CanRFP()) {
    searcher_.debug_info_.rfp_cuts++;
    return position_info_.static_eval;
  }

  auto &current_position = GetCurrentPosition();

  if (CanNullMove()) {
    searcher_.debug_info_.nmp_tries++;

    current_position.DoMove(NullMove{});

    const auto eval_optional = StartSubsearch<false>(
        {max_depth,
         static_cast<Depth>(
             remaining_depth -
             Settings::PruneParameters::NMPSettings::kNullMoveReduction),
         -beta, -beta + 1, true});

    current_position.UndoMove(NullMove{}, position_info_.irreversible_data);

    if (!eval_optional) return std::nullopt;

    const auto &null_eval = -*eval_optional;
    if (null_eval >= beta) {
      searcher_.debug_info_.nmp_cuts++;
      return beta;
    }
  }

  if (auto result = CheckTranspositionTable()) {
    searcher_.debug_info_.tt_cuts++;
    return *result;
  }

  auto const &move_generator = searcher_.move_generator_;

  move_picker_.InitPicker(
      move_generator.GenerateMoves<MoveGenerator::Type::kAll>(current_position),
      searcher_);

  // check if there are no possible moves
  if (move_picker_.Done()) {
    return GetEndGameScore();
  }

  if (!iteration_status_.best_move) {
    auto has_cutoff_opt = CheckFirstMove<is_principal_variation>(
        *move_picker_.SelectNextMove(searcher_, max_depth - remaining_depth));
    if (!has_cutoff_opt) {
      return std::nullopt;
    }
    if (*has_cutoff_opt) {
      SetTTEntry(Bound::kLower);
      return beta;
    }
  } else {
    // skip the first move
    assert(iteration_status_.best_move);
    move_picker_.SkipMove(*iteration_status_.best_move);
  }

  return PVSearch();
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult
SearchNode<is_principal_variation, ExitCondition>::QuiescenceSearch() {
  auto &current_position = GetCurrentPosition();
  auto quiescence_searcher = Quiescence{exit_condition_};

  const auto eval =
      quiescence_searcher.template Search</* start of quiscence search */ true>(
          current_position, state_.alpha, state_.beta, 0);

  searcher_.debug_info_.quiescence_nodes +=
      quiescence_searcher.GetSearchedNodes();

  return eval;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
Eval SearchNode<is_principal_variation, ExitCondition>::GetEndGameScore()
    const {
  if (position_info_.is_under_check) {
    return kMateValue +
           static_cast<Eval>(state_.max_depth - state_.remaining_depth);
  }

  return kDrawValue;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
void SearchNode<is_principal_variation, ExitCondition>::SetBestMove(Move move) {
  if (state_.remaining_depth == state_.max_depth) {
    searcher_.best_move_ = move;
  }
  iteration_status_.best_move = std::move(move);
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
void SearchNode<is_principal_variation, ExitCondition>::SetTTEntry(
    const Bound bound) {
  assert(iteration_status_.best_move);
  searcher_.best_moves_.SetEntry(
      GetCurrentPosition(), *iteration_status_.best_move,
      iteration_status_.best_eval +
          IsMateScore(iteration_status_.best_eval) *
              (state_.max_depth - state_.remaining_depth),
      state_.remaining_depth, bound, searcher_.age_);
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_pv_move>
SearchResult SearchNode<is_principal_variation, ExitCondition>::ProbeMove(
    const Move &move) {
  auto &current_position = GetCurrentPosition();
  auto &[max_depth, remaining_depth, alpha, beta, _] = state_;

  // make the move and search the tree
  DLOG(INFO) << std::string(max_depth - remaining_depth, '\t') << move;
  current_position.DoMove(move);

  assert((is_pv_move && is_principal_variation) == is_pv_move);
  const auto eval_optional =
      StartSubsearch < is_pv_move &&
      is_principal_variation >
          ({max_depth, static_cast<Depth>(remaining_depth - 1), -beta, -alpha});

  // undo the move
  current_position.UndoMove(move, position_info_.irreversible_data);

  if (!eval_optional) return std::nullopt;

  return eval_optional;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_pv_move>
std::optional<bool>
SearchNode<is_principal_variation, ExitCondition>::CheckFirstMove(
    const Move &move) {
  assert((is_pv_move && is_principal_variation) == is_pv_move);
  assert(!iteration_status_.best_move);

  const auto eval_optional =
      ProbeMove < is_pv_move && is_principal_variation > (move);
  if (!eval_optional) {
    return std::nullopt;
  }
  SetBestMove(move);
  iteration_status_.best_eval = -*eval_optional;

  auto &[max_depth, remaining_depth, alpha, beta, _] = state_;

  if (iteration_status_.best_eval > alpha) {
    if (iteration_status_.best_eval >= beta) {
      assert(iteration_status_.best_move);
      if (IsQuiet(*iteration_status_.best_move)) {
        UpdateQuietMove<true>(*iteration_status_.best_move);
      }

      return true;
    }

    iteration_status_.has_raised_alpha = true;
    alpha = iteration_status_.best_eval;
  }

  return false;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SearchNode<is_principal_variation, ExitCondition>::PVSearch() {
  auto &current_position = GetCurrentPosition();
  auto &[max_depth, remaining_depth, alpha, beta, _] = state_;

  const bool lmr_conditions_met =
      Settings::PruneParameters::LMRSettings::kEnabled &&
      !is_principal_variation &&
      remaining_depth >= Settings::PruneParameters::LMRSettings::kDepthLimit;

  for (auto it =
           move_picker_.SelectNextMove(searcher_, max_depth - remaining_depth);
       it != move_picker_.end(); it = move_picker_.SelectNextMove(
                                     searcher_, max_depth - remaining_depth)) {
    const auto &move = *it;

    DLOG(INFO) << std::string(max_depth - remaining_depth, '\t') << move;

    current_position.DoMove(move);  // make the move and search the tree

    // Late Move Reduction
    int R = 0;
    if (lmr_conditions_met &&
        move_picker_.GetCurrentStage() >= MovePicker::Stage::kQuiet) {
      R = 0.99 +
          std::log(remaining_depth) *
              std::log(move_picker_.current() - move_picker_.begin()) / 3.14 -
          (position_info_.is_under_check
               ? Settings::PruneParameters::LMRSettings::
                     kUnderCheckReductionPenalty
               : 0) -
          (current_position.IsUnderCheck()
               ? Settings::PruneParameters::LMRSettings::
                     kDoingCheckReductionPenalty
               : 0);
      R = std::clamp(R, 0, static_cast<int>(remaining_depth - 1));
    }

    auto temp_eval_optional = StartSubsearch<false>(
        {max_depth, static_cast<Depth>(remaining_depth - 1 - R), -alpha - 1,
         -alpha});  // Reduced ZWS

    if (!temp_eval_optional) {
      current_position.UndoMove(move, position_info_.irreversible_data);
      return std::nullopt;
    }

    auto temp_eval = -*temp_eval_optional;
    if (R > 0 &&
        temp_eval >
            alpha) { /* research at full depth, but still with zero window */
      temp_eval_optional = StartSubsearch<false>(
          {max_depth, static_cast<Depth>(remaining_depth - 1), -alpha - 1,
           -alpha});

      if (!temp_eval_optional) {
        current_position.UndoMove(move, position_info_.irreversible_data);
        return std::nullopt;
      }

      temp_eval = -*temp_eval_optional;
    }

    assert(is_principal_variation || (beta - alpha == 1));
    if (is_principal_variation &&
        temp_eval > alpha) /* make a research (ZWS failed) */
    {
      searcher_.debug_info_.zws_researches++;
      temp_eval_optional = StartSubsearch<true>(
          {max_depth, static_cast<Depth>(remaining_depth - 1), -beta, -alpha});
      if (!temp_eval_optional) {
        current_position.UndoMove(move, position_info_.irreversible_data);
        return std::nullopt;
      }

      temp_eval = -*temp_eval_optional;
    }

    if (temp_eval > alpha) {
      iteration_status_.has_raised_alpha = true;
      alpha = temp_eval;
    }

    // undo the move
    current_position.UndoMove(move, position_info_.irreversible_data);

    if (temp_eval > iteration_status_.best_eval) {
      SetBestMove(move);

      // check if we have found a better move
      if (temp_eval >= beta) {
        // beta-cutoff occurs, node is cut-type, returned score is
        // lower-bound
        SetTTEntry(Bound::kLower);

        if (move_picker_.GetCurrentStage() == MovePicker::Stage::kKillers ||
            move_picker_.GetCurrentStage() == MovePicker::Stage::kQuiet) {
          UpdateQuietMove<false>(move);
        }

        return beta;
      }

      iteration_status_.best_eval = temp_eval;
    }
  }

  // no beta-cutoff occured, so it is either an all node or a pv node
  SetTTEntry(iteration_status_.has_raised_alpha ? Bound::kExact
                                                : Bound::kUpper);

  return alpha;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<is_principal_variation, ExitCondition>::CanNullMove() const {
  if constexpr (!Settings::PruneParameters::NMPSettings::kEnabled) {
    return false;
  }

  const auto remaining_depth = state_.remaining_depth;

  if (remaining_depth <=
      Settings::PruneParameters::NMPSettings::kNullMoveReduction)
    return false;

  if (position_info_.is_under_check) return false;

  const auto &current_position = searcher_.GetPosition();
  const auto side_to_move = current_position.GetSideToMove();
  const auto king_and_pawns =
      current_position.GetPiecesByType<Piece::kKing>(side_to_move) |
      current_position.GetPiecesByType<Piece::kPawn>(side_to_move);

  return current_position.GetPieces(side_to_move) != king_and_pawns;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_first_move>
void SearchNode<is_principal_variation, ExitCondition>::UpdateQuietMove(
    const Move &move) {
  // history malus
  if constexpr (!is_first_move) {
    for (auto it = move_picker_.begin_quiet(); it != move_picker_.current();
         ++it) {
      const auto [from, to, captured_piece] = GetMoveData(*it);
      searcher_.history_[position_info_.side_to_move_idx][from][to] -=
          state_.remaining_depth * state_.remaining_depth;
    }
  }

  // history bonus
  const auto [from, to, captured_piece] = GetMoveData(move);
  searcher_.history_[position_info_.side_to_move_idx][from][to] +=
      state_.remaining_depth * state_.remaining_depth;

  // killer heuristic
  searcher_.killers_.TryAdd(state_.max_depth - state_.remaining_depth, move);
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
Position &
SearchNode<is_principal_variation, ExitCondition>::GetCurrentPosition() {
  return searcher_.current_position_;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<is_principal_variation, ExitCondition>::CanRFP() const {
  if constexpr (!Settings::PruneParameters::RFPSettings::kEnabled) return false;

  if constexpr (is_principal_variation) return false;

  return !position_info_.is_under_check &&
         state_.remaining_depth <=
             Settings::PruneParameters::RFPSettings::kDepthLimit &&
         position_info_.static_eval >
             state_.beta + Settings::PruneParameters::RFPSettings::kThreshold *
                               state_.remaining_depth;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<is_principal_variation,
                ExitCondition>::ProbeTranspositionTable() {
  auto node = searcher_.best_moves_.GetNode(searcher_.current_position_);
  if (node.true_hash == GetCurrentPosition().GetHash()) {
    iteration_status_.tt_info = std::move(node);
    return true;
  }
  return false;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
std::optional<SearchResult>
SearchNode<is_principal_variation, ExitCondition>::CheckTranspositionTable() {
  if (iteration_status_.tt_info) {
    auto &[max_depth, remaining_depth, alpha, beta, _] = state_;
    auto [hash, hash_move, entry_score, entry_depth, entry_bound, _] =
        *iteration_status_.tt_info;

    searcher_.debug_info_.tt_hits++;

    if (remaining_depth == max_depth) {
      searcher_.best_move_ = hash_move;
    }

    entry_score -= IsMateScore(entry_score) * (max_depth - remaining_depth);

    if (!is_principal_variation && entry_depth >= remaining_depth) {
      if (static_cast<Bound>(entry_bound) & Bound::kUpper &&
          entry_score <= alpha) {
        return alpha;
      }

      if (static_cast<Bound>(entry_bound) & Bound::kLower &&
          entry_score > alpha) {
        if (entry_score >= beta) {
          if (IsQuiet(hash_move)) {
            UpdateQuietMove<true>(hash_move);
          }

          // TODO: update entry age

          return beta;
        }

        iteration_status_.has_raised_alpha = true;
        alpha = entry_score;
      }

      if (static_cast<Bound>(entry_bound) == Bound::kExact) {
        return entry_score;
      }
    }

    auto has_cutoff_opt = CheckFirstMove<is_principal_variation>(hash_move);
    if (!has_cutoff_opt) {
      return SearchResult{std::nullopt};
    }
    if (*has_cutoff_opt) {
      SetTTEntry(Bound::kLower);
      return beta;
    }
  }

  return std::nullopt;
}
}  // namespace SimpleChessEngine
