#pragma once
#include <glog/logging.h>

#include <array>
#include <cstddef>

#include "Concepts.h"
#include "Evaluation.h"
#include "KillerTable.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "MovePicker.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine {
/**
 * \brief A simple implementation of the alpha-beta search algorithm.
 *
 * \details
 * A list of features:
 *  - PVS
 *  - ZWS
 *  - Fail-soft
 *  - Transposition table
 *  - Iterative deepening
 *  - Quiescence search
 *
 *  \author nook0110
 */
class Searcher {
 public:
  constexpr static size_t kTTsize = 1 << 24;
  using TranspositionTable = TranspositionTable<kTTsize>;

  struct PruneParameters {
    struct rfp {
      static constexpr bool kEnabled = true;
      static constexpr Depth kDepthLimit = 5;
      static constexpr Eval kThreshold = 75;
    };
    struct NullMove {
      static constexpr bool kEnabled = true;
      static constexpr size_t kNullMoveReduction = 3;
    };
  };

  struct DebugInfo {
    std::size_t searched_nodes{};
    std::size_t quiescence_nodes{};

    std::size_t zws_researches{};

    std::size_t rfp_cuts{};
    std::size_t nmp_tries{};
    std::size_t nmp_cuts{};

    std::size_t tt_hits{};
    std::size_t tt_cuts{};

    DebugInfo &operator+=(const DebugInfo &other) {
      searched_nodes += other.searched_nodes;
      quiescence_nodes += other.quiescence_nodes;
      zws_researches += other.zws_researches;
      rfp_cuts += other.rfp_cuts;
      nmp_tries += other.nmp_tries;
      nmp_cuts += other.nmp_cuts;
      tt_hits += other.tt_hits;
      tt_cuts += other.tt_cuts;
      return *this;
    }
  };

  /**
   * \brief Constructor.
   *
   * \param position The initial position.
   */
  explicit Searcher(Position position = PositionFactory{}())
      : current_position_(std::move(position)) {}

  /**
   * \brief Sets the current position.
   *
   * \param position New position.
   */
  void SetPosition(Position position);

  /**
   * \brief Returns the current position.
   *
   * \return The current position.
   */
  [[nodiscard]] const Position &GetPosition() const;

  /**
   * \brief Returns the best move for the current position.
   *
   * \return The current best move.
   */
  [[nodiscard]] const Move &GetCurrentBestMove() const;

  /**
   * \brief Performs the alpha-beta search algorithm.
   *
   * \param max_depth max_depth for search
   * \param remaining_depth The remaining depth.
   * \param alpha The current alpha value.
   * \param beta The current beta value.
   *
   * \return Evaluation of subtree.
   */
  template <bool is_principal_variation, class ExitCondition>
    requires StopSearchCondition<ExitCondition>
  [[nodiscard]] SearchResult Search(const ExitCondition &stop_search_condition,
                                    Depth max_depth, Depth remaining_depth,
                                    Eval alpha, Eval beta);
  void InitStartOfSearch();

  [[nodiscard]] const DebugInfo &GetInfo() const { return debug_info_; }
  [[nodiscard]] const auto &GetKillers() const { return killers_; }
  [[nodiscard]] const auto &GetHistory() const { return history_; }

  [[nodiscard]] MoveGenerator::Moves GetPrincipalVariation(
      Depth max_depth, Position position) const;

 private:
  struct SearchState {
    const Depth max_depth;
    const Depth remaining_depth;
    Eval alpha = {};
    const Eval beta;

    bool was_previous_move_a_null = false;
  };

  struct IterationStatus {
    bool has_stored_move = false;
    bool has_raised_alpha = false;
    Move best_move;
    Eval best_eval = {};
  };

  struct PositionInfo {
    PositionInfo(const Position &position)
        : static_eval(position.Evaluate()),
          is_under_check(position.IsUnderCheck()),
          irreversible_data(position.GetIrreversibleData()),
          side_to_move_idx(static_cast<size_t>(position.GetSideToMove())) {}
    const Eval static_eval;
    const bool is_under_check = false;
    const Position::IrreversibleData irreversible_data;
    const size_t side_to_move_idx;
  };

  template <bool is_principal_variation, class ExitCondition>
    requires StopSearchCondition<ExitCondition>
  struct SearchImplementation {
   public:
    constexpr static size_t kEnoughNodesToCheckTime = 1 << 12;

    SearchImplementation(Searcher &searcher, SearchState state,
                         const ExitCondition &exit_condition);

    template <bool is_principal_variation_search>
    SearchResult Search(SearchState state);

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

    SearchResult QuiescenceSearch();
    Eval GetEndGameScore() const;

    void SetBestMove(const Move &move);
    void SetTTEntry(const Bound bound);
    void UpdateQuietMove(const Move &move);

    template <bool is_pv_move>
    SearchResult ProbeMove(const Move &move);
    template <bool is_pv_move>
    std::optional<bool> CheckFirstMove(const Move &move);

    [[nodiscard]] bool CanRFP() const;

    std::optional<SearchResult> CheckTranspositionTable();

    SearchResult PVSearch();

    [[nodiscard]] bool CanNullMove() const;

    Searcher &searcher_;
  };

  Age age_{};
  Move best_move_{};
  Position current_position_;     //!< Current position.
  MoveGenerator move_generator_;  //!< Move generator.
  TranspositionTable
      best_moves_;  //!< Transposition-table to store the best moves.
  std::array<std::array<std::array<uint64_t, kBoardArea + 1>, kBoardArea + 1>,
             kColors>
      history_ = {};
  KillerTable<2> killers_;

  DebugInfo debug_info_;
};
}  // namespace SimpleChessEngine

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////

namespace SimpleChessEngine {
inline void Searcher::SetPosition(Position position) {
  current_position_ = std::move(position);
}

inline const Position &Searcher::GetPosition() const {
  return current_position_;
}

inline const Move &Searcher::GetCurrentBestMove() const { return best_move_; }

inline MoveGenerator::Moves Searcher::GetPrincipalVariation(
    Depth max_depth, Position position) const {
  MoveGenerator::Moves answer;
  for (Depth i = 0; i < max_depth; ++i) {
    const auto &hashed_node = best_moves_.GetNode(position);
    if (hashed_node.true_hash != position.GetHash()) break;
    position.DoMove(hashed_node.move);
    answer.push_back(hashed_node.move);
  }
  return answer;
}

inline void Searcher::InitStartOfSearch() {
  killers_.Clear();
  for (size_t color = 0; color < kColors; ++color) {
    for (BitIndex from = 0; from <= kBoardArea; ++from) {
      history_[color][from].fill(0ull);
    }
  }
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline SearchResult SimpleChessEngine::Searcher::Search(
    const ExitCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta) {
  debug_info_ = DebugInfo{};
  ++age_;

  return SearchImplementation<is_principal_variation, ExitCondition>{
      *this,
      {max_depth, remaining_depth, alpha, beta},
      stop_search_condition}();
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation,
    ExitCondition>::SearchImplementation(Searcher &searcher, SearchState state,
                                         const ExitCondition &exit_condition)
    : state_(state), exit_condition_(exit_condition),
      position_info_(searcher.GetPosition()), searcher_(searcher) {}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_principal_variation_search>
inline SearchResult
Searcher::SearchImplementation<is_principal_variation, ExitCondition>::Search(
    SearchState state) {
  DLOG(INFO) << "PV: " << std::boolalpha << is_principal_variation_search;
  return SearchImplementation<is_principal_variation_search, ExitCondition>{
      searcher_, state, exit_condition_}();
}
template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline bool SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::IsTimeToExit() const {
  return searcher_.debug_info_.searched_nodes % kEnoughNodesToCheckTime == 0 &&
         exit_condition_.IsTimeToExit();
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline SearchResult SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::operator()() {
  if (IsTimeToExit()) {
    return std::nullopt;
  }

  if (searcher_.current_position_.DetectRepetition()) {
    return kDrawValue;
  }

  auto &[max_depth, remaining_depth, alpha, beta, _] = state_;

  // return the evaluation of the current position if we have reached
  // the end of the search tree
  if (remaining_depth <= 0) {
    return QuiescenceSearch();
  }

  searcher_.debug_info_.searched_nodes++;

  if (auto result = CheckTranspositionTable()) {
    searcher_.debug_info_.tt_cuts++;
    return *result;
  }

  if (CanRFP()) {
    searcher_.debug_info_.rfp_cuts++;
    return position_info_.static_eval;
  }

  auto &current_position = searcher_.current_position_;

  if (CanNullMove()) {
    searcher_.debug_info_.nmp_tries++;
    current_position.DoMove(NullMove{});
    const auto eval_optional = Search<false>(
        {max_depth,
         static_cast<Depth>(remaining_depth -
                            PruneParameters::NullMove::kNullMoveReduction),
         -beta, -beta + 1, true});

    if (!eval_optional) return std::nullopt;

    current_position.UndoMove(NullMove{}, position_info_.irreversible_data);

    const auto &null_eval = -*eval_optional;
    if (null_eval >= beta) {
      searcher_.debug_info_.nmp_cuts++;
      return null_eval;
    }
  }

  auto const &move_generator = searcher_.move_generator_;

  move_picker_.InitPicker(
      move_generator.GenerateMoves<MoveGenerator::Type::kDefault>(
          current_position),
      searcher_);

  // check if there are no possible moves
  if (!move_picker_.HasMoreMoves()) {
    return GetEndGameScore();
  }

  if (!iteration_status_.has_stored_move) {
    auto has_cutoff_opt = CheckFirstMove<false>(
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
    move_picker_.SkipMove(iteration_status_.best_move);
  }

  return PVSearch();
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline SearchResult SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::QuiescenceSearch() {
  auto &current_position = searcher_.current_position_;
  auto quiescence_searcher = Quiescence{exit_condition_};

  const auto eval = quiescence_searcher.template Search<true>(
      current_position, state_.alpha, state_.beta);
  searcher_.debug_info_.quiescence_nodes +=
      quiescence_searcher.GetSearchedNodes();
  return eval;
}
template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline Eval SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::GetEndGameScore() const {
  if (position_info_.is_under_check) {
    return kMateValue +
           static_cast<Eval>(state_.max_depth - state_.remaining_depth);
  }
  return kDrawValue;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline void SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::SetBestMove(const Move &move) {
  auto &current_position = searcher_.current_position_;

  iteration_status_.best_move = move;

  if (state_.remaining_depth == state_.max_depth) {
    searcher_.best_move_ = move;
  }
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline void SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::SetTTEntry(const Bound bound) {
  searcher_.best_moves_.SetEntry(
      searcher_.current_position_, iteration_status_.best_move,
      iteration_status_.best_eval +
          IsMateScore(iteration_status_.best_eval) *
              (state_.max_depth - state_.remaining_depth),
      state_.remaining_depth, bound, searcher_.age_);
}
template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_pv_move>
inline SearchResult Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::ProbeMove(const Move &move) {
  auto &current_position = searcher_.current_position_;
  auto &[max_depth, remaining_depth, alpha, beta, _] = state_;

  // make the move and search the tree
  DLOG(INFO) << std::string(max_depth - remaining_depth, '\t') << move;
  current_position.DoMove(move);

  const auto eval_optional =
      Search < is_pv_move &&
      is_principal_variation >
          ({max_depth, static_cast<Depth>(remaining_depth - 1), -beta, -alpha});
  if (!eval_optional) return std::nullopt;

  // undo the move
  current_position.UndoMove(move, position_info_.irreversible_data);

  return eval_optional;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
template <bool is_pv_move>
inline std::optional<bool> SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::CheckFirstMove(const Move &move) {
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
      if (IsQuiet(iteration_status_.best_move)) {
        UpdateQuietMove(iteration_status_.best_move);
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
inline SearchResult SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::PVSearch() {
  auto &current_position = searcher_.current_position_;
  auto &[max_depth, remaining_depth, alpha, beta, _] = state_;

  bool is_quiet = false;
  for (auto it =
           move_picker_.SelectNextMove(searcher_, max_depth - remaining_depth);
       it != move_picker_.end(); it = move_picker_.SelectNextMove(
                                     searcher_, max_depth - remaining_depth)) {
    const auto &move = *it;

    is_quiet = move_picker_.GetCurrentStage() == MovePicker::Stage::kQuiet;

    DLOG(INFO) << std::string(max_depth - remaining_depth, '\t') << move;
    current_position.DoMove(move);  // make the move and search the tree

    auto temp_eval_optional =
        Search<false>({max_depth, static_cast<Depth>(remaining_depth - 1),
                       -alpha - 1, -alpha});  // ZWS

    if (!temp_eval_optional) return std::nullopt;

    auto temp_eval = -*temp_eval_optional;

    if (temp_eval > alpha &&
        is_principal_variation) /* make a research (ZWS failed) */
    {
      searcher_.debug_info_.zws_researches++;
      temp_eval_optional = Search<true>(
          {max_depth, static_cast<Depth>(remaining_depth - 1), -beta, -alpha});
      if (!temp_eval_optional) return std::nullopt;

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
        if (is_quiet) {
          UpdateQuietMove(move);
        }
        return beta;
      }

      iteration_status_.best_eval = temp_eval;
    }
  }

  SetTTEntry(iteration_status_.has_raised_alpha ? Bound::kExact
                                                : Bound::kUpper);
  return alpha;
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline bool SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::CanNullMove() const {
  if constexpr (!PruneParameters::NullMove::kEnabled) {
    return false;
  }
  const auto max_depth = state_.max_depth;
  const auto remaining_depth = state_.remaining_depth;
  const auto was_previous_move_a_null = state_.was_previous_move_a_null;
  const auto beta = state_.beta;

  if (remaining_depth <= PruneParameters::NullMove::kNullMoveReduction)
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
inline void SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::UpdateQuietMove(const Move &move) {
  const auto [from, to, captured_piece] = GetMoveData(move);
  searcher_.history_[position_info_.side_to_move_idx][from][to] +=
      state_.remaining_depth * state_.remaining_depth;
  searcher_.killers_.TryAdd(state_.max_depth - state_.remaining_depth, move);
}
template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline bool Searcher::SearchImplementation<is_principal_variation,
                                           ExitCondition>::CanRFP() const {
  if constexpr (!PruneParameters::rfp::kEnabled) return false;
  if constexpr (is_principal_variation) return false;
  return !position_info_.is_under_check &&
         state_.remaining_depth <= PruneParameters::rfp::kDepthLimit &&
         position_info_.static_eval >
             state_.beta +
                 PruneParameters::rfp::kThreshold * state_.remaining_depth;
}
template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
inline std::optional<SearchResult> Searcher::SearchImplementation<
    is_principal_variation, ExitCondition>::CheckTranspositionTable() {
  if (auto [hash, hash_move, entry_score, entry_depth, entry_bound, _] =
          searcher_.best_moves_.GetNode(searcher_.current_position_);
      hash == searcher_.current_position_
                  .GetHash()) {  // check if current position was previously
                                 // searched at higher depth
    auto &[max_depth, remaining_depth, alpha, beta, __] = state_;
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
            UpdateQuietMove(hash_move);
          }
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
    iteration_status_.has_stored_move = true;
  }
  return std::nullopt;
}
}  // namespace SimpleChessEngine
