#pragma once

#include <cassert>

#include "Concepts.h"
#include "Evaluation.h"
#include "Move.h"
#include "MovePicker.h"
#include "NodeType.h"
#include "Position.h"
#include "StaticEvalHistory.h"
#include "TranspositionTable.h"
#include "Utility.h"

namespace SimpleChessEngine {
class Searcher;
enum class Bound : std::uint8_t;

struct SearchState {
  const Depth max_depth;
  Depth remaining_depth;
  Eval alpha = {};
  const Eval beta;
  Depth ply = 0;

  bool was_previous_move_a_null = false;
};

struct IterationStatus {
  bool has_raised_alpha = false;
  std::optional<Move> best_move;
  std::optional<Node> tt_info;
  Eval best_eval = {};
};

struct PositionInfo {
  explicit PositionInfo(const Position &position);
  mutable std::optional<Eval> static_eval;
  const bool is_under_check = false;
  const Position::IrreversibleData irreversible_data;
  const size_t side_to_move_idx;
};

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
struct SearchNode {
  friend struct SearchEvaluationTestAccess;

 public:
  constexpr static bool kIsPrincipalVariation = node_type == NodeType::kPV;
  constexpr static NodeType kFirstChildNodeExpectedType =
      node_type == NodeType::kPV    ? NodeType::kPV
      : node_type == NodeType::kCut ? NodeType::kAll
      : node_type == NodeType::kAll ? NodeType::kCut
                                    : NodeType::kInvalid;
  constexpr static NodeType kZWSNodeType =
      node_type == NodeType::kPV    ? NodeType::kCut
      : node_type == NodeType::kCut ? NodeType::kAll
      : node_type == NodeType::kAll ? NodeType::kCut
                                    : NodeType::kInvalid;

  SearchNode(Searcher &searcher, SearchState state,
             const ExitCondition &exit_condition,
             std::optional<Move> excluded_move = std::nullopt);

  template <NodeType expected_node_type>
  SearchResult StartSubsearch(SearchState state);

  bool IsTimeToExit() const;

  SearchResult operator()();

 private:
  friend struct ProbCutTestAccess;

  /* Search args */
  SearchState state_;
  const std::optional<Move> excluded_move_;
  int tt_move_extension_ = 0;

  const ExitCondition &exit_condition_;

  /* Local variables for search */
  IterationStatus iteration_status_;
  PositionInfo position_info_;
  EvaluationTrend evaluation_trend_;
  MovePicker move_picker_;
  std::array<Move, 64> searched_quiets_{};
  size_t searched_quiet_count_ = 0;

  SearchResult QuiescenceSearch();
  Eval GetEndGameScore() const;
  Eval GetStaticEval() const;

  void SetBestMove(Move move);
  void SetTTEntry(const Bound bound);
  template <bool is_first_move>
  void UpdateQuietMove(const Move &move);
  void RecordSearchedQuiet(const Move &move);

  Position &GetCurrentPosition();

  template <NodeType expected_node_type>
  SearchResult ProbeMove(const LegalMove &move);
  template <NodeType expected_node_type>
  std::optional<bool> CheckFirstMove(const LegalMove &move);

  [[nodiscard]] bool CanRFP() const;
  [[nodiscard]] bool CanLateMovePrune() const;
  [[nodiscard]] bool CanFutilityPrune() const;
  [[nodiscard]] bool PassesSEEPruning(const Move &move, bool is_quiet) const;

  std::optional<SearchResult> CheckProbCut();
  bool ProbeTranspositionTable();
  std::optional<SearchResult> CheckSingularExtension();

  SearchResult PVSearch();

  [[nodiscard]] bool CanNullMove() const;

  Searcher &searcher_;
};
}  // namespace SimpleChessEngine

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////
#include "Quiescence.h"
#include "Searcher.h"
#include "Settings.h"

namespace SimpleChessEngine {
inline PositionInfo::PositionInfo(const Position &position)
    : is_under_check(position.IsUnderCheck()),
      irreversible_data(position.GetIrreversibleData()),
      side_to_move_idx(static_cast<size_t>(position.GetSideToMove())) {}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchNode<node_type, ExitCondition>::SearchNode(
    Searcher &searcher, SearchState state, const ExitCondition &exit_condition,
    std::optional<Move> excluded_move)
    : state_(state), excluded_move_(excluded_move),
      exit_condition_(exit_condition),
      position_info_(searcher.GetPosition()), searcher_(searcher) {
  assert(IsValidNodeType<kFirstChildNodeExpectedType>());
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <NodeType expected_node_type>
SearchResult SearchNode<node_type, ExitCondition>::StartSubsearch(
    SearchState state) {
  assert(expected_node_type == NodeType::kPV ||
         (state.beta - state.alpha == 1));
  assert(state.alpha < state.beta);

  assert(expected_node_type != NodeType::kPV || node_type == NodeType::kPV);
  assert(expected_node_type != NodeType::kCut ||
         (node_type == NodeType::kPV || node_type == NodeType::kAll));
  assert(expected_node_type != NodeType::kAll || node_type == NodeType::kCut);

  return SearchNode<expected_node_type, ExitCondition>{searcher_, state,
                                                       exit_condition_}();
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::IsTimeToExit() const {
  return exit_condition_.IsTimeToExit();
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SearchNode<node_type, ExitCondition>::operator()() {
  if (IsTimeToExit()) {
    return std::nullopt;
  }

  auto &[max_depth, remaining_depth, alpha, beta, ply, _] = state_;
  assert(kIsPrincipalVariation || beta - alpha == 1);

  if (ply > 0 && GetCurrentPosition().DetectRepetition(ply)) {
    return kDrawValue;
  }

  if (ply > 0 && GetCurrentPosition().GetHalfMoveClock() >= 100 &&
      (!position_info_.is_under_check ||
       searcher_.move_generator_.HasLegalMove(GetCurrentPosition()))) {
    return kDrawValue;
  }

  if (ply >= kMaxSearchPly - 1) {
    return GetStaticEval();
  }

  if (!excluded_move_ && position_info_.is_under_check && ply < 2 * max_depth) {
    ++remaining_depth;
  }

  // return the evaluation of the current position if we have reached
  // the end of the search tree
  if (remaining_depth <= 0) {
    return QuiescenceSearch();
  }

  if (ply > 0 && GetCurrentPosition().IsInsufficientMaterial()) {
    return kDrawValue;
  }

  if (!excluded_move_ && ply > 0 && searcher_.tablebase_ &&
      GetCurrentPosition().GetAllPieces().Count() <=
          searcher_.tablebase_->MaxPieces()) {
    if (const auto wdl =
            searcher_.tablebase_->ProbeWdl(GetCurrentPosition())) {
      if (*wdl == Tablebase::Wdl::kWin) {
        return kTablebaseWinValue - ply;
      }
      if (*wdl == Tablebase::Wdl::kLoss) {
        return -kTablebaseWinValue + ply;
      }
      return kDrawValue;
    }
  }

  searcher_.debug_info_.searched_nodes++;

  if (ProbeTranspositionTable()) {
    searcher_.debug_info_.tt_hits++;
    auto [hash, hash_move, entry_score, entry_depth, entry_bound, _, cached_eval] =
        *iteration_status_.tt_info;
    entry_score = FromTTScore(entry_score, ply);

    if (!excluded_move_ && !kIsPrincipalVariation &&
        entry_depth >= remaining_depth) {
      if (static_cast<Bound>(entry_bound) & Bound::kUpper &&
          entry_score <= alpha) {
        return alpha;
      }

      if (static_cast<Bound>(entry_bound) & Bound::kLower &&
          entry_score > alpha) {
        if (entry_score >= beta) {
          if (hash_move.IsQuiet(searcher_.GetPosition())) {
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
  }

  evaluation_trend_ = searcher_.static_evals_.Update(
      ply, position_info_.is_under_check ? std::nullopt
                                         : std::optional{GetStaticEval()});
  if (!excluded_move_ && !position_info_.is_under_check) {
    searcher_.UpdateStaticEvalHistory(ply, *position_info_.static_eval);
  }

  if (CanRFP()) {
    searcher_.debug_info_.rfp_cuts++;
    return GetStaticEval();
  }

  auto &current_position = GetCurrentPosition();

  if (CanNullMove()) {
    searcher_.debug_info_.nmp_tries++;

    using NMPSettings = Settings::PruneParameters::NMPSettings;
    const int reduction =
        NMPSettings::kNullMoveReduction +
        remaining_depth / NMPSettings::kDepthDivisor +
        std::max((GetStaticEval() - beta) / NMPSettings::kEvalDivisor, 0);
    const auto null_depth =
        static_cast<Depth>(std::max(0, remaining_depth - reduction));

    searcher_.SetPlayedNullMove(ply);
    current_position.DoMove(NullMove{});

    const auto eval_optional =
        StartSubsearch<NodeType::kAll>({max_depth, null_depth, -beta, -beta + 1,
                                        static_cast<Depth>(ply + 1), true});

    current_position.UndoMove(NullMove{}, position_info_.irreversible_data);

    if (!eval_optional) return std::nullopt;

    const auto &null_eval = -*eval_optional;
    if (null_eval >= beta &&
        null_eval < kTablebaseWinValue - static_cast<Eval>(kMaxSearchPly)) {
      if (searcher_.nmp_min_ply_ ||
          remaining_depth < NMPSettings::kVerificationDepth) {
        searcher_.debug_info_.nmp_cuts++;
        return beta;
      }

      searcher_.nmp_min_ply_ = static_cast<Depth>(ply + 3 * null_depth / 4);
      ++searcher_.null_move_info_.verifications;
      const auto verification = SearchNode<NodeType::kAll, ExitCondition>{
          searcher_,
          {max_depth, null_depth, beta - 1, beta, ply,
           state_.was_previous_move_a_null},
          exit_condition_}();
      searcher_.nmp_min_ply_ = 0;

      if (!verification) return std::nullopt;
      if (*verification >= beta) {
        searcher_.debug_info_.nmp_cuts++;
        return beta;
      }
      ++searcher_.null_move_info_.verification_fail_lows;
    }
  }

  if (!position_info_.is_under_check) {
    evaluation_trend_.improving |=
        GetStaticEval() >=
        beta + Settings::SearchParameters::kImprovingBetaMargin;
  }

  // Internal Iterative Reduction
  if constexpr (node_type == NodeType::kPV || node_type == NodeType::kCut) {
    if ((!iteration_status_.tt_info ||
         !iteration_status_.tt_info->move.IsValid()) &&
        remaining_depth > Settings::PruneParameters::IIRSettings::kReduction &&
        remaining_depth >=
            Settings::PruneParameters::IIRSettings::kBaseLimit +
                (node_type == NodeType::kCut
                     ? Settings::PruneParameters::IIRSettings::kCutNodePenalty
                     : 0)) {
      remaining_depth -= Settings::PruneParameters::IIRSettings::kReduction;
    }
  }

  if (auto result = CheckProbCut()) return *result;

  const auto tt_move = !excluded_move_ && iteration_status_.tt_info
                           ? iteration_status_.tt_info->move
                           : Move::None();
  move_picker_.InitPicker(current_position, &searcher_, ply, tt_move,
                          excluded_move_.value_or(Move::None()));
  const auto first_move = move_picker_.SelectNextMove();
  if (first_move.IsValid() && first_move == tt_move) {
    if (auto result = CheckSingularExtension()) {
      searcher_.debug_info_.tt_cuts++;
      return *result;
    }
  }
  if (!first_move.IsValid()) {
    if (excluded_move_) return alpha;
    return GetEndGameScore();
  }
  auto has_cutoff_opt = CheckFirstMove<kFirstChildNodeExpectedType>(*first_move);
  if (!has_cutoff_opt) return std::nullopt;
  if (*has_cutoff_opt) {
    if (iteration_status_.tt_info &&
        first_move == iteration_status_.tt_info->move) {
      searcher_.debug_info_.tt_cuts++;
    }
    SetTTEntry(Bound::kLower);
    return beta;
  }

  return PVSearch();
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
std::optional<SearchResult>
SearchNode<node_type, ExitCondition>::CheckProbCut() {
  if constexpr (kIsPrincipalVariation) return std::nullopt;

  using ProbCutSettings = Settings::PruneParameters::ProbCutSettings;
  constexpr Eval decisive = kTablebaseWinValue - kMaxSearchPly;
  if (state_.ply == 0 || position_info_.is_under_check || excluded_move_ ||
      state_.remaining_depth < ProbCutSettings::kMinDepth ||
      state_.beta <= -decisive || state_.beta >= decisive)
    return std::nullopt;

  const Eval probcut_beta =
      state_.beta + (evaluation_trend_.improving
                         ? ProbCutSettings::kImprovingMargin
                         : ProbCutSettings::kMargin);
  if (probcut_beta >= decisive) return std::nullopt;

  const auto child_depth = static_cast<Depth>(int{state_.remaining_depth} -
                                              ProbCutSettings::kDepthReduction);
  if (iteration_status_.tt_info) {
    const auto &entry = *iteration_status_.tt_info;
    const Eval value = FromTTScore(entry.score, state_.ply);
    if (entry.depth >= child_depth + 1 && value > -decisive &&
        value < decisive && value < probcut_beta)
      return std::nullopt;
  }

  ++searcher_.debug_info_.probcut_tries;
  auto &position = GetCurrentPosition();
  move_picker_.InitProbCut(position, &searcher_, state_.ply);
  const Eval see_threshold = probcut_beta - GetStaticEval();
  for (auto next = move_picker_.SelectProbCutMove(see_threshold); next;
       next = move_picker_.SelectProbCutMove(see_threshold)) {
    const auto& move = *next;
    ++searcher_.debug_info_.probcut_candidates;
    searcher_.SetPlayedMove(
        state_.ply, move,
        excluded_move_ ? std::nullopt : position_info_.static_eval);
    position.DoMove(move);

    auto quiescence = Quiescence{exit_condition_, searcher_.best_moves_.get(),
                                 searcher_.age_};
    const auto qvalue = quiescence.template Search<true>(
        position, -probcut_beta, -probcut_beta + 1,
        static_cast<Depth>(state_.ply + 1), false);
    searcher_.debug_info_.quiescence_nodes += quiescence.GetSearchedNodes();
    searcher_.debug_info_.probcut_qsearch_nodes +=
        quiescence.GetSearchedNodes();

    SearchResult value = qvalue;
    if (qvalue && -*qvalue >= probcut_beta) {
      ++searcher_.debug_info_.probcut_qsearch_passes;
      const auto nodes_before = searcher_.debug_info_.searched_nodes +
                                searcher_.debug_info_.quiescence_nodes;
      value = StartSubsearch<kZWSNodeType>(
          {state_.max_depth, child_depth, -probcut_beta, -probcut_beta + 1,
           static_cast<Depth>(state_.ply + 1)});
      searcher_.debug_info_.probcut_verification_nodes +=
          searcher_.debug_info_.searched_nodes +
          searcher_.debug_info_.quiescence_nodes - nodes_before;
    }

    position.UndoMove(move, position_info_.irreversible_data);
    if (!value || IsTimeToExit()) return SearchResult{};
    if (-*value >= probcut_beta) {
      ++searcher_.debug_info_.probcut_cuts;
      return SearchResult{state_.beta};
    }
  }
  return std::nullopt;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SearchNode<node_type, ExitCondition>::QuiescenceSearch() {
  auto &current_position = GetCurrentPosition();
  auto quiescence_searcher =
      Quiescence{exit_condition_, searcher_.best_moves_.get(), searcher_.age_};

  const auto eval =
      quiescence_searcher
          .template Search</* start of the quiescence search */ true>(
              current_position, state_.alpha, state_.beta, state_.ply,
              kIsPrincipalVariation);

  searcher_.debug_info_.quiescence_nodes +=
      quiescence_searcher.GetSearchedNodes();

  return eval;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
Eval SearchNode<node_type, ExitCondition>::GetEndGameScore() const {
  if (position_info_.is_under_check) {
    return kMateValue + static_cast<Eval>(state_.ply);
  }

  return kDrawValue;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
Eval SearchNode<node_type, ExitCondition>::GetStaticEval() const {
  if (!position_info_.static_eval) {
    Eval cached_eval = kNoStaticEval;
    if (iteration_status_.tt_info && Nnue::IsLoaded()) {
      const auto context =
          (static_cast<std::uint64_t>(Nnue::Generation()) << 32) |
          static_cast<std::uint32_t>(
              Settings::EvaluationParameters::nnue_scale);
      cached_eval = searcher_.best_moves_->GetStaticEval(
          *iteration_status_.tt_info, context);
    }
    position_info_.static_eval = cached_eval != kNoStaticEval
                                     ? cached_eval
                                     : searcher_.GetPosition().Evaluate();
  }
  return *position_info_.static_eval;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
void SearchNode<node_type, ExitCondition>::SetBestMove(Move move) {
  if (state_.ply == 0) {
    searcher_.best_move_ = move;
  }
  iteration_status_.best_move = std::move(move);
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
void SearchNode<node_type, ExitCondition>::SetTTEntry(const Bound bound) {
  if (excluded_move_) return;
  assert(iteration_status_.best_move);
  searcher_.best_moves_->SetEntry(
      GetCurrentPosition(), *iteration_status_.best_move,
      ToTTScore(iteration_status_.best_eval, state_.ply),
      state_.remaining_depth, bound, searcher_.age_,
      Nnue::IsLoaded() ? position_info_.static_eval.value_or(kNoStaticEval)
                       : kNoStaticEval,
      (static_cast<std::uint64_t>(Nnue::Generation()) << 32) |
          static_cast<std::uint32_t>(
              Settings::EvaluationParameters::nnue_scale));
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <NodeType expected_node_type>
SearchResult SearchNode<node_type, ExitCondition>::ProbeMove(const LegalMove &move) {
  auto &current_position = GetCurrentPosition();
  auto &[max_depth, remaining_depth, alpha, beta, ply, _] = state_;

  if (move.IsQuiet(current_position)) {
    RecordSearchedQuiet(move);
  }
  searcher_.SetPlayedMove(
      ply, move, excluded_move_ ? std::nullopt : position_info_.static_eval);
  const auto nodes_before = searcher_.debug_info_.searched_nodes +
                            searcher_.debug_info_.quiescence_nodes;

  // make the move and search the tree
  current_position.DoMove(move);

  static_assert(expected_node_type != NodeType::kPV ||
                expected_node_type == node_type);
  const auto eval_optional = StartSubsearch<expected_node_type>(
      {max_depth, static_cast<Depth>(
                      std::max(0, remaining_depth - 1 + tt_move_extension_)),
       -beta, -alpha,
       static_cast<Depth>(ply + 1)});

  // undo the move
  current_position.UndoMove(move, position_info_.irreversible_data);
  if (ply == 0) {
    searcher_.AddRootEffort(move, searcher_.debug_info_.searched_nodes +
                                      searcher_.debug_info_.quiescence_nodes -
                                      nodes_before);
  }

  if (!eval_optional) return std::nullopt;

  return eval_optional;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <NodeType expected_node_type>
std::optional<bool> SearchNode<node_type, ExitCondition>::CheckFirstMove(
    const LegalMove &move) {
  static_assert(expected_node_type == kFirstChildNodeExpectedType);
  static_assert(expected_node_type != NodeType::kPV ||
                expected_node_type == node_type);
  assert(!iteration_status_.best_move);

  const auto eval_optional = ProbeMove<expected_node_type>(move);
  if (!eval_optional) {
    return std::nullopt;
  }
  SetBestMove(move);
  iteration_status_.best_eval = -*eval_optional;

  auto &[max_depth, remaining_depth, alpha, beta, ply, _] = state_;

  if (iteration_status_.best_eval > alpha) {
    if (iteration_status_.best_eval >= beta) {
      assert(iteration_status_.best_move);
      if (iteration_status_.best_move->IsQuiet(searcher_.GetPosition())) {
        UpdateQuietMove<true>(*iteration_status_.best_move);
      }

      return true;
    }

    iteration_status_.has_raised_alpha = true;
    alpha = iteration_status_.best_eval;
  }

  return false;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SearchNode<node_type, ExitCondition>::PVSearch() {
  auto &current_position = GetCurrentPosition();
  auto &[max_depth, remaining_depth, alpha, beta, ply, _] = state_;

  const bool lmr_conditions_met =
      Settings::PruneParameters::LMRSettings::kEnabled &&
      remaining_depth >= Settings::PruneParameters::LMRSettings::kDepthLimit;

  const bool move_pruning_allowed =
      ply != 0 && !position_info_.is_under_check;

  for (auto next = move_picker_.SelectNextMove(); next;
       next = move_picker_.SelectNextMove()) {
    const auto& move = *next;
    const bool is_quiet = move.IsQuiet(current_position);
    const auto quiet_score =
        is_quiet ? searcher_.GetQuietScore(move, ply) : std::int64_t{0};
    const bool can_prune =
        move_pruning_allowed &&
        iteration_status_.best_eval >
            -kTablebaseWinValue + static_cast<Eval>(kMaxSearchPly);

    if (can_prune && is_quiet && CanLateMovePrune()) {
      searcher_.debug_info_.lmp_prunes++;
      continue;
    }

    if (can_prune && !PassesSEEPruning(move, is_quiet)) {
      searcher_.debug_info_.see_prunes++;
      continue;
    }

    const bool can_futility_prune =
        can_prune && is_quiet && CanFutilityPrune();

    searcher_.SetPlayedMove(
        ply, move, excluded_move_ ? std::nullopt : position_info_.static_eval);
    const auto nodes_before = searcher_.debug_info_.searched_nodes +
                              searcher_.debug_info_.quiescence_nodes;
    current_position.DoMove(move);  // make the move and search the tree

    if (can_futility_prune && !current_position.IsUnderCheck()) {
      current_position.UndoMove(move, position_info_.irreversible_data);
      searcher_.debug_info_.futility_prunes++;
      continue;
    }

    if (is_quiet) {
      RecordSearchedQuiet(move);
    }

    // Late Move Reduction
    int R = 0;
    if (lmr_conditions_met &&
        move_picker_.GetCurrentStage() >= MovePicker::Stage::kQuiet) {
      using LMRSettings = Settings::PruneParameters::LMRSettings;
      double reduction_scale =
          std::log(remaining_depth) * std::log(move_picker_.MoveCount()) / 3.14;
      if (alpha > -kTablebaseWinValue + static_cast<Eval>(kMaxSearchPly) &&
          alpha < kTablebaseWinValue - static_cast<Eval>(kMaxSearchPly)) {
        reduction_scale = LMRSettings::ScaleReduction(
            reduction_scale, evaluation_trend_.improving);
      }
      double reduction = 0.99 + reduction_scale -
                         (position_info_.is_under_check
                              ? LMRSettings::kUnderCheckReductionPenalty
                              : 0) -
                         (current_position.IsUnderCheck()
                              ? LMRSettings::kDoingCheckReductionPenalty
                              : 0);
      if constexpr (node_type == NodeType::kCut) {
        reduction += LMRSettings::kCutNodeReduction;
      }
      if constexpr (kIsPrincipalVariation) {
        reduction -= 1;
      }
      reduction -= static_cast<double>(quiet_score) /
                   static_cast<double>(LMRSettings::kHistoryDivisor);
      R = std::clamp(static_cast<int>(reduction), 0,
                     static_cast<int>(remaining_depth - 1));
    }

    auto temp_eval_optional = StartSubsearch<kZWSNodeType>(
        {max_depth, static_cast<Depth>(remaining_depth - 1 - R), -alpha - 1,
         -alpha, static_cast<Depth>(ply + 1)});  // Reduced ZWS

    if (!temp_eval_optional) {
      current_position.UndoMove(move, position_info_.irreversible_data);
      return std::nullopt;
    }

    auto temp_eval = -*temp_eval_optional;
    if (R > 0 &&
        temp_eval >
            alpha) { /* research at full depth, but still with zero window */
      temp_eval_optional = StartSubsearch<kZWSNodeType>(
          {max_depth, static_cast<Depth>(remaining_depth - 1), -alpha - 1,
           -alpha, static_cast<Depth>(ply + 1)});

      if (!temp_eval_optional) {
        current_position.UndoMove(move, position_info_.irreversible_data);
        return std::nullopt;
      }

      temp_eval = -*temp_eval_optional;
    }

    assert(kIsPrincipalVariation || (beta - alpha == 1));
    if (kIsPrincipalVariation &&
        temp_eval > alpha) /* make a research (ZWS failed) */
    {
      searcher_.debug_info_.zws_researches++;
      temp_eval_optional = StartSubsearch<NodeType::kPV>(
          {max_depth, static_cast<Depth>(remaining_depth - 1), -beta, -alpha,
           static_cast<Depth>(ply + 1)});
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
    if (ply == 0) {
      searcher_.AddRootEffort(move, searcher_.debug_info_.searched_nodes +
                                        searcher_.debug_info_.quiescence_nodes -
                                        nodes_before);
    }

    if (temp_eval > iteration_status_.best_eval) {
      SetBestMove(move);
      iteration_status_.best_eval = temp_eval;

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
    }
  }

  // no beta-cutoff occurred, so it is either an all node or a pv node
  SetTTEntry(iteration_status_.has_raised_alpha ? Bound::kExact
                                                : Bound::kUpper);

  return alpha;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::CanNullMove() const {
  if constexpr (!Settings::PruneParameters::NMPSettings::kEnabled) {
    return false;
  }

  if constexpr (node_type != NodeType::kCut) {
    return false;
  }

  if (excluded_move_ || state_.was_previous_move_a_null) return false;

  using NMPSettings = Settings::PruneParameters::NMPSettings;
  if (state_.ply < searcher_.nmp_min_ply_ ||
      state_.beta < NMPSettings::kMinBeta)
    return false;

  if (position_info_.is_under_check) return false;

  if ((GetStaticEval() - state_.beta) *
          NMPSettings::kStaticEvalMarginDenominator <
      NMPSettings::kStaticEvalBaseMarginNumerator -
          NMPSettings::kStaticEvalDepthMarginNumerator *
              state_.remaining_depth -
          NMPSettings::kStaticEvalImprovingMarginNumerator *
              evaluation_trend_.improving)
    return false;

  const auto &current_position = searcher_.GetPosition();
  const auto side_to_move = current_position.GetSideToMove();
  const auto king_and_pawns =
      current_position.GetPiecesByType<Piece::kKing>(side_to_move) |
      current_position.GetPiecesByType<Piece::kPawn>(side_to_move);

  return current_position.GetPieces(side_to_move) != king_and_pawns;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_first_move>
void SearchNode<node_type, ExitCondition>::UpdateQuietMove(const Move &move) {
  const auto bonus = Searcher::HistoryBonus(state_.remaining_depth);
  if constexpr (!is_first_move) {
    for (size_t i = 0; i < searched_quiet_count_; ++i) {
      if (searched_quiets_[i] != move) {
        searcher_.UpdateQuietHistory(searched_quiets_[i], state_.ply, -bonus);
      }
    }
  }
  searcher_.UpdateQuietHistory(move, state_.ply, bonus);

  searcher_.killers_.TryAdd(state_.ply, move);
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
void SearchNode<node_type, ExitCondition>::RecordSearchedQuiet(
    const Move &move) {
  if (searched_quiet_count_ < searched_quiets_.size()) {
    searched_quiets_[searched_quiet_count_++] = move;
  }
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
Position &SearchNode<node_type, ExitCondition>::GetCurrentPosition() {
  return searcher_.current_position_;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::CanRFP() const {
  using RFPSettings = Settings::PruneParameters::RFPSettings;
  if constexpr (!RFPSettings::kEnabled) return false;

  if constexpr (kIsPrincipalVariation) return false;

  if (position_info_.is_under_check ||
      state_.remaining_depth > RFPSettings::kDepthLimit ||
      state_.beta <= -kTablebaseWinValue + static_cast<Eval>(kMaxSearchPly)) {
    return false;
  }

  const Eval eval = GetStaticEval();
  if (eval < state_.beta ||
      eval >= kTablebaseWinValue - static_cast<Eval>(kMaxSearchPly) ||
      (iteration_status_.tt_info && iteration_status_.tt_info->move.IsValid() &&
       iteration_status_.tt_info->move.IsQuiet(searcher_.GetPosition()))) {
    return false;
  }

  return eval >= state_.beta +
                     RFPSettings::Margin(state_.remaining_depth,
                                         node_type == NodeType::kCut &&
                                             !iteration_status_.tt_info,
                                         evaluation_trend_.improving,
                                         evaluation_trend_.opponent_worsening);
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::CanLateMovePrune() const {
  using LMPSettings = Settings::PruneParameters::LMPSettings;
  if constexpr (!LMPSettings::kEnabled) return false;

  const int depth = state_.remaining_depth;
  if (depth > LMPSettings::kDepthLimit) return false;

  const auto move_count = move_picker_.MoveCount();
  return move_count >=
         LMPSettings::MoveCount(depth, evaluation_trend_.improving);
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::CanFutilityPrune() const {
  using FutilitySettings = Settings::PruneParameters::FutilitySettings;
  if constexpr (!FutilitySettings::kEnabled) return false;

  const Eval depth = state_.remaining_depth;
  if (depth > FutilitySettings::kDepthLimit) return false;

  return GetStaticEval() + FutilitySettings::kBaseMargin +
             FutilitySettings::kDepthMargin * depth <=
         state_.alpha;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::PassesSEEPruning(
    const Move &move, const bool is_quiet) const {
  using SEEPruningSettings = Settings::PruneParameters::SEEPruningSettings;
  if constexpr (!SEEPruningSettings::kEnabled) return true;

  const Eval depth = state_.remaining_depth;
  if (depth > SEEPruningSettings::kDepthLimit) return true;

  const Eval threshold =
      is_quiet ? -SEEPruningSettings::kQuietMargin * depth
               : -SEEPruningSettings::kNoisyMargin * depth * depth;
  return searcher_.GetPosition().StaticExchangeEvaluation(move, threshold);
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::ProbeTranspositionTable() {
  if (auto node = searcher_.best_moves_->GetNode(searcher_.current_position_);
      node.IsOccupied() && node.true_hash == GetCurrentPosition().GetHash()) {
    iteration_status_.tt_info = std::move(node);
    return true;
  }

  return false;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
std::optional<SearchResult>
SearchNode<node_type, ExitCondition>::CheckSingularExtension() {
  auto &[max_depth, remaining_depth, alpha, beta, ply, _] = state_;
  auto [hash, hash_move, entry_score, entry_depth, entry_bound, _, cached_eval] =
      *iteration_status_.tt_info;

  using Singular = Settings::SearchParameters::SingularExtensionSettings;
  entry_score = FromTTScore(entry_score, ply);
  if (ply > 0 && ply < 2 * max_depth &&
      remaining_depth >= Singular::kDepthLimit + kIsPrincipalVariation &&
      entry_depth >= remaining_depth - 3 &&
      (entry_bound & Bound::kLower) &&
      std::abs(entry_score) <
          kTablebaseWinValue - static_cast<Eval>(kMaxSearchPly)) {
    const Eval singular_beta =
        entry_score - Singular::kMarginNumerator * remaining_depth /
                          Singular::kMarginDenominator;
    constexpr auto verification_type =
        node_type == NodeType::kCut ? NodeType::kCut : NodeType::kAll;
    ++searcher_.singular_info_.searches;
    const auto result = SearchNode<verification_type, ExitCondition>{
        searcher_,
        {max_depth, static_cast<Depth>((remaining_depth - 1) / 2),
         singular_beta - 1, singular_beta, ply,
         state_.was_previous_move_a_null},
        exit_condition_, hash_move}();
    if (!result) return SearchResult{std::nullopt};
    if (*result < singular_beta) {
      tt_move_extension_ = 1;
      ++searcher_.singular_info_.extensions;
    } else if (singular_beta >= beta) {
      ++searcher_.singular_info_.multi_cuts;
      return beta;
    } else if (entry_score >= beta) {
      tt_move_extension_ = Singular::kNegativeExtension;
      ++searcher_.singular_info_.negative_extensions;
    }
  }

  return std::nullopt;
}
}  // namespace SimpleChessEngine
