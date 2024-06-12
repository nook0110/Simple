#pragma once
#include <algorithm>
#include <array>

#include "Evaluation.h"
#include "KillerTable.h"
#include "MoveGenerator.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "StreamUtility.h"
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
 *  - Aspiration windows
 *  - Transposition table
 *  - Iterative deepening
 *  - Quiescence search
 *
 *  \author nook0110
 */
class Searcher {
 public:
#ifdef _DEBUG
  constexpr static size_t kTTsize = 1 << 10;
#else
  constexpr static size_t kTTsize = 1 << 26;
#endif
  using TranspositionTable = TranspositionTable<kTTsize>;

  struct DebugInfo {
    std::size_t searched_nodes{};
    std::size_t quiescence_nodes{};
    std::size_t pv_hits{};

    DebugInfo &operator+=(const DebugInfo &other) {
      searched_nodes += other.searched_nodes;
      quiescence_nodes += other.quiescence_nodes;
      pv_hits += other.pv_hits;
      return *this;
    }
  };

  /**
   * \brief Constructor.
   *
   * \param position The initial position.
   */
  explicit Searcher(const Position position = PositionFactory{}())
      : current_position_(position), move_generator_(MoveGenerator()) {}

  /**
   * \brief Sets the current position.
   *
   * \param position New position.
   */
  void SetPosition(const Position &position);

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
  template <bool is_principal_variation>
  [[nodiscard]] SearchResult Search(const TimePoint &end_time, size_t max_depth,
                                    size_t remaining_depth, Eval alpha,
                                    Eval beta);

  [[nodiscard]] const DebugInfo &GetInfo() const { return debug_info_; }

  [[nodiscard]] std::size_t GetSearchedNodes() const {
    return debug_info_.searched_nodes;
  }

  [[nodiscard]] std::size_t GetPVHits() const { return debug_info_.pv_hits; }

  [[nodiscard]] MoveGenerator::Moves GetPrincipalVariation(
      std::size_t max_depth) const {
    return GetPrincipalVariation(max_depth, current_position_);
  }

  [[nodiscard]] MoveGenerator::Moves GetPrincipalVariation(
      std::size_t max_depth, Position position) const {
    MoveGenerator::Moves answer;
    for (std::size_t i = 0; i < max_depth; ++i) {
      const auto &hashed_node = best_moves_.GetNode(position);
      if (hashed_node.true_hash != position.GetHash()) break;
      position.DoMove(hashed_node.move);
      answer.push_back(hashed_node.move);
    }
    return answer;
  }

  void InitStartOfSearch();

 private:
  template <bool is_principal_variation>
  struct SearchImplementation {
   public:
    constexpr static size_t kEnoughNodesToCheckTime = 1 << 12;

    SearchImplementation(Searcher &searcher, const size_t max_depth,
                         const size_t remaining_depth, const Eval alpha,
                         const Eval beta, const TimePoint &end_time);

    template <bool is_principal_variation_search>
    SearchResult Search(const size_t max_depth, const size_t remaining_depth,
                        Eval alpha, const Eval beta);

    bool IsTimeToExit() const;

    SearchResult operator()();

    /* Search args */
    const size_t max_depth;
    const size_t remaining_depth;
    Eval alpha;
    const Eval beta;
    const TimePoint &end_time;

    /* Local variables for search */
    bool has_stored_move = false;
    bool has_raised_alpha = false;
    Move best_move;
    Eval best_eval;
    const Position::IrreversibleData irreversible_data;
    const size_t side_to_move_idx;

   private:
    SearchResult QuiescenceSearch();

    Eval GetEndGameScore() const;

    void SetBestMove(const Move &move);

    void SetTTEntry(const Bound bound);

    template <bool is_pv_move>
    SearchResult ProbeMove(const Move &move);

    template <bool is_pv_move>
    std::optional<bool> CheckFirstMove(const Move &move);

    struct QuietRange {
      MoveGenerator::Moves::iterator quiet_begin;
      MoveGenerator::Moves::iterator quiet_end;
    };

    QuietRange OrderMoves(const MoveGenerator::Moves::iterator first,
                          const MoveGenerator::Moves::iterator last);

    SearchResult PVSearch(const MoveGenerator::Moves::iterator first,
                          const MoveGenerator::Moves::iterator last,
                          const QuietRange &ordeing_result);

    void UpdateQuietMove(const Move &move);

    Searcher &searcher_;
  };

  std::pair<MoveGenerator::Moves::iterator, MoveGenerator::Moves::iterator>
  OrderMoves(const MoveGenerator::Moves::iterator first,
             const MoveGenerator::Moves::iterator last, const size_t ply,
             const Player color) const;
  Age age_;

  Move best_move_;

  Position current_position_;  //!< Current position.

  MoveGenerator move_generator_;  //!< Move generator.

  TranspositionTable
      best_moves_;  //!< Transposition-table to store the best moves.

  std::array<std::array<std::array<uint64_t, kBoardArea + 1>, kBoardArea + 1>,
             kColors>
      history_;

  KillerTable<2> killers_;

  DebugInfo debug_info_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
inline void Searcher::SetPosition(const Position &position) {
  current_position_ = position;
}

inline const Position &Searcher::GetPosition() const {
  return current_position_;
}

inline const Move &Searcher::GetCurrentBestMove() const { return best_move_; }

inline void Searcher::InitStartOfSearch() {
  killers_.Clear();
  for (unsigned color = 0; color < kColors; ++color) {
    for (BitIndex from = 0; from <= kBoardArea; ++from) {
      for (BitIndex to = 0; to <= kBoardArea; ++to) {
        history_[color][from][to] = 0ull;
      }
    }
  }
}

template <bool is_principal_variation>
SearchResult Searcher::Search(const TimePoint &end_time, const size_t max_depth,
                              const size_t remaining_depth, Eval alpha,
                              const Eval beta) {
  debug_info_ = DebugInfo{};

  ++age_;

  return SearchImplementation<is_principal_variation>{
      *this, max_depth, remaining_depth, alpha, beta, end_time}();
}

inline std::pair<MoveGenerator::Moves::iterator, MoveGenerator::Moves::iterator>
Searcher::OrderMoves(const MoveGenerator::Moves::iterator first,
                     const MoveGenerator::Moves::iterator last,
                     const size_t ply, const Player color) const {
  auto color_idx = static_cast<size_t>(color);
  MoveGenerator::Moves::iterator quiet_begin;
  quiet_begin = std::partition(first, last, [this](const Move &move) {
    if (std::holds_alternative<Promotion>(move) ||
        std::holds_alternative<EnCroissant>(move))
      return true;
    if (!std::holds_alternative<DefaultMove>(move)) return false;
    const auto [from, to, captured_piece] = std::get<DefaultMove>(move);
    return static_cast<size_t>(captured_piece) >=
           static_cast<size_t>(current_position_.GetPiece(from));
  });
  MoveGenerator::Moves::iterator quiet_end;
  quiet_end = std::partition(quiet_begin, last, [](const Move &move) {
    if (!std::holds_alternative<DefaultMove>(move)) return true;
    const auto [from, to, captured_piece] = std::get<DefaultMove>(move);
    return !captured_piece;
  });
  const auto CompareCaptures = [this](const Move &lhs, const Move &rhs) {
    const auto [from_lhs, to_lhs, captured_piece_lhs] = GetMoveData(lhs);
    const auto [from_rhs, to_rhs, captured_piece_rhs] = GetMoveData(rhs);
    const auto captured_idx_lhs = static_cast<int>(captured_piece_lhs);
    const auto captured_idx_rhs = static_cast<int>(captured_piece_rhs);
    const auto moving_idx_lhs =
        -static_cast<int>(current_position_.GetPiece(from_lhs));
    const auto moving_idx_rhs =
        -static_cast<int>(current_position_.GetPiece(from_rhs));
    return std::tie(captured_idx_lhs, moving_idx_lhs) >
           std::tie(captured_idx_rhs, moving_idx_rhs);
  };
  std::sort(first, quiet_begin, CompareCaptures);
  std::sort(quiet_end, last, CompareCaptures);
  int increment = 0;
  for (size_t i = 0; i < killers_.AvailableKillerCount(ply); ++i) {
    const auto killer = killers_.Get(ply, i);
    if (auto it = std::find(quiet_begin, quiet_end, killer); it != quiet_end) {
      std::iter_swap(quiet_begin, it);
      ++quiet_begin;
      ++increment;
    }
  }
  const auto CompareQuiet = [this, color_idx](const Move &lhs,
                                              const Move &rhs) {
    const auto [from_lhs, to_lhs, captured_piece_lhs] = GetMoveData(lhs);
    const auto [from_rhs, to_rhs, captured_piece_rhs] = GetMoveData(rhs);
    return history_[color_idx][from_lhs][to_lhs] >
           history_[color_idx][from_rhs][to_rhs];
  };
  std::sort(quiet_begin - increment, quiet_begin, CompareQuiet);
  std::sort(quiet_begin, quiet_end, CompareQuiet);
  std::advance(quiet_begin, -increment);
  return {quiet_begin, quiet_end};
}
template <bool is_principal_variation>
inline SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::SearchImplementation(Searcher &searcher,
                                                  const size_t max_depth,
                                                  const size_t remaining_depth,
                                                  const Eval alpha,
                                                  const Eval beta,
                                                  const TimePoint &end_time)
    : max_depth(max_depth),
      remaining_depth(remaining_depth),
      alpha(alpha),
      beta(beta),
      end_time(end_time),
      irreversible_data(searcher.current_position_.GetIrreversibleData()),
      side_to_move_idx(
          static_cast<size_t>(searcher.current_position_.GetSideToMove())),
      searcher_(searcher) {}

template <bool is_principal_variation>
template <bool is_principal_variation_search>
inline SearchResult
Searcher::SearchImplementation<is_principal_variation>::Search(
    const size_t max_depth, const size_t remaining_depth, Eval alpha,
    const Eval beta) {
  return SearchImplementation<is_principal_variation_search>{
      searcher_, max_depth, remaining_depth, alpha, beta, end_time}();
}
template <bool is_principal_variation>
inline bool SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::IsTimeToExit() const {
  return searcher_.debug_info_.searched_nodes % kEnoughNodesToCheckTime == 0 &&
         std::chrono::system_clock::now() > end_time;
}

template <bool is_principal_variation>
inline SearchResult SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::operator()() {
  if (IsTimeToExit()) {
    return std::nullopt;
  }

  if (searcher_.current_position_.DetectRepetition()) {
    return kDrawValue;
  }

  if constexpr (is_principal_variation) {
    if (remaining_depth <= 1) {
      return Search<false>(max_depth, remaining_depth, alpha, beta);
    }
  }

  // return the evaluation of the current position if we have reached
  // the end of the search tree
  if (remaining_depth <= 0) {
    return QuiescenceSearch();
  }

  searcher_.debug_info_.searched_nodes++;

  if (auto [hash, hash_move, entry_score, entry_depth, entry_bound, _] =
          searcher_.best_moves_.GetNode(searcher_.current_position_);
      hash == searcher_.current_position_
                  .GetHash()) {  // check if current position was previously
                                 // searched at higher depth

    if (remaining_depth == max_depth) {
      searcher_.best_move_ = hash_move;
    }

    entry_score -= IsMateScore(entry_score) * (max_depth - remaining_depth);

    if (entry_depth >= remaining_depth) {
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
        has_raised_alpha = true;
        alpha = entry_score;
      }
      if (static_cast<Bound>(entry_bound) == Bound::kExact) {
        return entry_score;
      }
    }
    auto has_cutoff_opt = CheckFirstMove<is_principal_variation>(hash_move);
    if (!has_cutoff_opt) {
      return std::nullopt;
    }
    if (*has_cutoff_opt) {
      SetTTEntry(Bound::kLower);
      return beta;
    }
    has_stored_move = true;
  }

  if constexpr (is_principal_variation) {
    //assert(has_stored_move);
  }

  auto &move_generator = searcher_.move_generator_;
  auto &current_position = searcher_.current_position_;

  auto moves = move_generator.GenerateMoves<MoveGenerator::Type::kDefault>(
      current_position);

  // check if there are no possible moves
  if (moves.empty()) {
    return GetEndGameScore();
  }

  const auto ordering_result = OrderMoves(moves.begin(), moves.end());

  if (!has_stored_move) {
    auto has_cutoff_opt = CheckFirstMove<false>(moves.front());
    if (!has_cutoff_opt) {
      return std::nullopt;
    }
    if (*has_cutoff_opt) {
      SetTTEntry(Bound::kLower);
      return beta;
    }
  }

  return PVSearch(std::next(moves.begin()), moves.end(), ordering_result);
}

template <bool is_principal_variation>
inline SearchResult SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::QuiescenceSearch() {
  auto &current_position = searcher_.current_position_;
  auto quiescence_searcher = Quiescence{end_time};

  const auto eval =
      quiescence_searcher.Search<true>(current_position, alpha, beta);
  searcher_.debug_info_.quiescence_nodes +=
      quiescence_searcher.GetSearchedNodes();
  return eval;
}
template <bool is_principal_variation>
inline Eval SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::GetEndGameScore() const {
  if (searcher_.current_position_.IsUnderCheck()) {
    return kMateValue + static_cast<Eval>(max_depth - remaining_depth);
  }
  return kDrawValue;
}

template <bool is_principal_variation>
inline void SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::SetBestMove(const Move &move) {
  auto &current_position = searcher_.current_position_;

  best_move = move;

  if (remaining_depth == max_depth) {
    searcher_.best_move_ = move;
  }
}

template <bool is_principal_variation>
inline void SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::SetTTEntry(const Bound bound) {
  searcher_.best_moves_.SetEntry(
      searcher_.current_position_, best_move,
      best_eval + IsMateScore(best_eval) * (max_depth - remaining_depth),
      remaining_depth, bound, searcher_.age_);
}
template <bool is_principal_variation>
template <bool is_pv_move>
inline SearchResult
Searcher::SearchImplementation<is_principal_variation>::ProbeMove(
    const Move &move) {
  auto &current_position = searcher_.current_position_;
  const Hash hash = current_position.GetHash();

  // make the move and search the tree
  current_position.DoMove(move);

  const auto eval_optional = Search<is_pv_move>(max_depth, remaining_depth - 1, -beta, -alpha);

  // undo the move
  current_position.UndoMove(move, irreversible_data);

  return eval_optional;
}

template <bool is_principal_variation>
template <bool is_pv_move>
inline std::optional<bool> SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::CheckFirstMove(const Move &move) {
  const auto eval_optional = ProbeMove<is_pv_move>(move);
  if (!eval_optional) {
    return std::nullopt;
  }
  const auto &eval = -*eval_optional;

  SetBestMove(move);
  best_eval = eval;

  if (best_eval > alpha) {
    if (best_eval >= beta) {
      if (IsQuiet(best_move)) {
        UpdateQuietMove(best_move);
      }
      return true;
    }
    has_raised_alpha = true;
    alpha = best_eval;
  }

  return false;
}
template <bool is_principal_variation>
inline Searcher::SearchImplementation<is_principal_variation>::QuietRange
SimpleChessEngine::Searcher::SearchImplementation<is_principal_variation>::
    OrderMoves(const MoveGenerator::Moves::iterator first,
               const MoveGenerator::Moves::iterator last) {
  auto &current_position = searcher_.current_position_;
  auto &best_moves = searcher_.best_moves_;

  auto begin_of_ordering = first;

  if (has_stored_move) {
    std::iter_swap(std::find(first, last, best_move), begin_of_ordering++);
  }

  auto [quiet_begin, quiet_end] =
      searcher_.OrderMoves(begin_of_ordering, last, max_depth - remaining_depth,
                           current_position.GetSideToMove());
  return QuietRange{quiet_begin, quiet_end};
}
template <bool is_principal_variation>
inline SearchResult SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::PVSearch(const MoveGenerator::Moves::iterator
                                          first,
                                      const MoveGenerator::Moves::iterator last,
                                      const QuietRange &ordeing_result) {
  auto &current_position = searcher_.current_position_;

  bool is_quiet = false;
  for (auto it = first; it != last; ++it) {
    const auto &move = *it;

    if (it >= ordeing_result.quiet_begin) is_quiet = true;
    if (it >= ordeing_result.quiet_end) is_quiet = false;

    current_position.DoMove(move);  // make the move and search the tree

    auto temp_eval_optional = Search<false>(max_depth, remaining_depth - 1,
                                            -alpha - 1, -alpha);  // ZWS

    if (!temp_eval_optional) return std::nullopt;

    auto temp_eval = -*temp_eval_optional;

    if (temp_eval > alpha) /* make a research (ZWS failed) */
    {
      temp_eval_optional = Search<false>(max_depth, remaining_depth - 1, -beta, -alpha);
      if (!temp_eval_optional) return std::nullopt;

      temp_eval = -*temp_eval_optional;

      if (temp_eval > alpha) {
        has_raised_alpha = true;
        alpha = temp_eval;
      }
    }

    // undo the move
    current_position.UndoMove(move, irreversible_data);

    if (temp_eval > best_eval) {
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

      best_eval = temp_eval;
    }
  }

  SetTTEntry(has_raised_alpha ? Bound::kExact : Bound::kUpper);
  return alpha;
}

template <bool is_principal_variation>
inline void SimpleChessEngine::Searcher::SearchImplementation<
    is_principal_variation>::UpdateQuietMove(const Move &move) {
  const auto [from, to, captured_piece] = GetMoveData(move);
  searcher_.history_[side_to_move_idx][from][to] +=
      remaining_depth * remaining_depth;
  searcher_.killers_.TryAdd(max_depth - remaining_depth, move);
}
}  // namespace SimpleChessEngine
