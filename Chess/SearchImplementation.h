#pragma once

#include <cassert>

#include "Concepts.h"
#include "Evaluation.h"
#include "Move.h"
#include "MovePicker.h"
#include "NodeType.h"
#include "Position.h"
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
  const Eval static_eval;
  const bool is_under_check = false;
  const Position::IrreversibleData irreversible_data;
  const size_t side_to_move_idx;
};

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
struct SearchNode {
 public:
  constexpr static size_t kEnoughNodesToCheckTime = 1 << 12;
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
             const ExitCondition &exit_condition);

  template <NodeType expected_node_type>
  SearchResult StartSubsearch(SearchState state);

  bool IsTimeToExit() const;

  SearchResult operator()();

 private:
  /* Search args */
  SearchState state_;

  const ExitCondition &exit_condition_;

  /* Local variables for search */
  IterationStatus iteration_status_;
  PositionInfo position_info_;
  MovePicker move_picker_;
  std::array<Move, 64> searched_quiets_{};
  size_t searched_quiet_count_ = 0;

  SearchResult QuiescenceSearch();
  Eval GetEndGameScore() const;

  void SetBestMove(Move move);
  void SetTTEntry(const Bound bound);
  template <bool is_first_move>
  void UpdateQuietMove(const Move &move);
  void RecordSearchedQuiet(const Move &move);

  Position &GetCurrentPosition();

  template <NodeType expected_node_type>
  SearchResult ProbeMove(const Move &move);
  template <NodeType expected_node_type>
  std::optional<bool> CheckFirstMove(const Move &move);

  [[nodiscard]] bool CanRFP() const;
  [[nodiscard]] bool CanLateMovePrune() const;
  [[nodiscard]] bool CanFutilityPrune() const;
  [[nodiscard]] bool PassesSEEPruning(const Move &move, bool is_quiet) const;

  bool ProbeTranspositionTable();
  std::optional<SearchResult> CheckTranspositionTable(
      const MoveGenerator::Moves &moves);

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
    : static_eval(position.Evaluate()), is_under_check(position.IsUnderCheck()),
      irreversible_data(position.GetIrreversibleData()),
      side_to_move_idx(static_cast<size_t>(position.GetSideToMove())) {}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchNode<node_type, ExitCondition>::SearchNode(
    Searcher &searcher, SearchState state, const ExitCondition &exit_condition)
    : state_(state), exit_condition_(exit_condition),
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
  const auto searched_nodes = searcher_.debug_info_.searched_nodes;
  return searched_nodes != 0 && searched_nodes % kEnoughNodesToCheckTime == 0 &&
         exit_condition_.IsTimeToExit();
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
       !searcher_.move_generator_
            .GenerateMoves<MoveGenerator::Type::kAll>(GetCurrentPosition())
            .empty())) {
    return kDrawValue;
  }

  if (ply >= kMaxSearchPly - 1) {
    return position_info_.static_eval;
  }

  if (position_info_.is_under_check && ply < 2 * max_depth) {
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

  if (ply > 0 && searcher_.tablebase_ &&
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
    auto [hash, hash_move, entry_score, entry_depth, entry_bound, _] =
        *iteration_status_.tt_info;
    entry_score -= IsMateScore(entry_score) * ply;

    if (!kIsPrincipalVariation && entry_depth >= remaining_depth) {
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

  if (CanRFP()) {
    searcher_.debug_info_.rfp_cuts++;
    return position_info_.static_eval;
  }

  auto &current_position = GetCurrentPosition();

  if (CanNullMove()) {
    searcher_.debug_info_.nmp_tries++;

    searcher_.SetPlayedNullMove(ply);
    current_position.DoMove(NullMove{});

    const auto eval_optional = StartSubsearch<NodeType::kAll>(
        {max_depth,
         static_cast<Depth>(
             remaining_depth -
             Settings::PruneParameters::NMPSettings::kNullMoveReduction - 1),
         -beta, -beta + 1, static_cast<Depth>(ply + 1), true});

    current_position.UndoMove(NullMove{}, position_info_.irreversible_data);

    if (!eval_optional) return std::nullopt;

    const auto &null_eval = -*eval_optional;
    if (null_eval >= beta) {
      searcher_.debug_info_.nmp_cuts++;
      return beta;
    }
  }

  // Internal Iterative Reduction
  if constexpr (node_type == NodeType::kPV || node_type == NodeType::kCut) {
    if (!iteration_status_.tt_info &&
        remaining_depth >
            Settings::PruneParameters::IIRSettings::kReduction &&
        remaining_depth >=
            Settings::PruneParameters::IIRSettings::kBaseLimit +
                (node_type == NodeType::kCut
                     ? Settings::PruneParameters::IIRSettings::kCutNodePenalty
                     : 0)) {
      remaining_depth -= Settings::PruneParameters::IIRSettings::kReduction;
    }
  }

  auto moves = searcher_.move_generator_
                   .GenerateMoves<MoveGenerator::Type::kAll>(current_position);

  if (auto result = CheckTranspositionTable(moves)) {
    searcher_.debug_info_.tt_cuts++;
    return *result;
  }

  move_picker_.InitPicker(std::move(moves), searcher_, ply);

  // check if there are no possible moves
  if (move_picker_.Done()) {
    return GetEndGameScore();
  }

  if (!iteration_status_.best_move) {
    auto has_cutoff_opt = CheckFirstMove<kFirstChildNodeExpectedType>(
        *move_picker_.SelectNextMove(searcher_, ply));
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

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SearchNode<node_type, ExitCondition>::QuiescenceSearch() {
  auto &current_position = GetCurrentPosition();
  auto quiescence_searcher = Quiescence{exit_condition_};

  const auto eval =
      quiescence_searcher
          .template Search</* start of the quiescence search */ true>(
              current_position, state_.alpha, state_.beta, state_.ply);

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
void SearchNode<node_type, ExitCondition>::SetBestMove(Move move) {
  if (state_.ply == 0) {
    if (iteration_status_.best_move && *iteration_status_.best_move != move) {
      ++searcher_.best_move_changes_;
    }
    searcher_.best_move_ = move;
  }
  iteration_status_.best_move = std::move(move);
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
void SearchNode<node_type, ExitCondition>::SetTTEntry(const Bound bound) {
  assert(iteration_status_.best_move);
  searcher_.best_moves_->SetEntry(
      GetCurrentPosition(), *iteration_status_.best_move,
      iteration_status_.best_eval +
          IsMateScore(iteration_status_.best_eval) * state_.ply,
      state_.remaining_depth, bound, searcher_.age_);
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <NodeType expected_node_type>
SearchResult SearchNode<node_type, ExitCondition>::ProbeMove(const Move &move) {
  auto &current_position = GetCurrentPosition();
  auto &[max_depth, remaining_depth, alpha, beta, ply, _] = state_;

  if (move.IsQuiet(current_position)) {
    RecordSearchedQuiet(move);
  }
  searcher_.SetPlayedMove(ply, move);
  const auto nodes_before = searcher_.debug_info_.searched_nodes +
                            searcher_.debug_info_.quiescence_nodes;

  // make the move and search the tree
  current_position.DoMove(move);

  static_assert(expected_node_type != NodeType::kPV ||
                expected_node_type == node_type);
  const auto eval_optional = StartSubsearch<expected_node_type>(
      {max_depth, static_cast<Depth>(remaining_depth - 1), -beta, -alpha,
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
    const Move &move) {
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
      !kIsPrincipalVariation &&
      remaining_depth >= Settings::PruneParameters::LMRSettings::kDepthLimit;

  const bool move_pruning_allowed =
      ply != 0 && !position_info_.is_under_check;

  for (auto it = move_picker_.SelectNextMove(searcher_, ply);
       it != move_picker_.end();
       it = move_picker_.SelectNextMove(searcher_, ply)) {
    const auto &move = *it;

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

    searcher_.SetPlayedMove(ply, move);
    current_position.DoMove(move);  // make the move and search the tree

    if (can_prune && is_quiet && !current_position.IsUnderCheck() &&
        CanFutilityPrune()) {
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
      double reduction =
          0.99 +
          std::log(remaining_depth) *
              std::log(move_picker_.current() - move_picker_.begin()) / 3.14 -
          (position_info_.is_under_check
               ? LMRSettings::kUnderCheckReductionPenalty
               : 0) -
          (current_position.IsUnderCheck()
               ? LMRSettings::kDoingCheckReductionPenalty
               : 0);
      if constexpr (node_type == NodeType::kCut) {
        reduction += LMRSettings::kCutNodeReduction;
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

  if (state_.was_previous_move_a_null) return false;

  const auto remaining_depth = state_.remaining_depth;

  if (remaining_depth <=
      Settings::PruneParameters::NMPSettings::kNullMoveReduction)
    return false;

  if (position_info_.is_under_check) return false;

  if (position_info_.static_eval < state_.beta) return false;

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
  if constexpr (!Settings::PruneParameters::RFPSettings::kEnabled) return false;

  if constexpr (kIsPrincipalVariation) return false;

  return !position_info_.is_under_check &&
         state_.remaining_depth <=
             Settings::PruneParameters::RFPSettings::kDepthLimit &&
         position_info_.static_eval >
             state_.beta + Settings::PruneParameters::RFPSettings::kThreshold *
                               state_.remaining_depth;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::CanLateMovePrune() const {
  using LMPSettings = Settings::PruneParameters::LMPSettings;
  if constexpr (!LMPSettings::kEnabled) return false;

  const int depth = state_.remaining_depth;
  if (depth > LMPSettings::kDepthLimit) return false;

  const auto move_count = move_picker_.current() - move_picker_.begin();
  return move_count >= LMPSettings::kBaseCount + depth * depth;
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
bool SearchNode<node_type, ExitCondition>::CanFutilityPrune() const {
  using FutilitySettings = Settings::PruneParameters::FutilitySettings;
  if constexpr (!FutilitySettings::kEnabled) return false;

  const Eval depth = state_.remaining_depth;
  if (depth > FutilitySettings::kDepthLimit) return false;

  return position_info_.static_eval + FutilitySettings::kBaseMargin +
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
SearchNode<node_type, ExitCondition>::CheckTranspositionTable(
    const MoveGenerator::Moves &moves) {
  if (iteration_status_.tt_info &&
      std::ranges::find(moves, iteration_status_.tt_info->move) !=
          moves.end()) {
    auto &[max_depth, remaining_depth, alpha, beta, ply, _] = state_;
    auto [hash, hash_move, entry_score, entry_depth, entry_bound, _] =
        *iteration_status_.tt_info;

    auto has_cutoff_opt =
        CheckFirstMove<kFirstChildNodeExpectedType>(hash_move);
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
