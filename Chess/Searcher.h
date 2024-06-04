#pragma once
#include <algorithm>

#include "Evaluation.h"
#include "KillerTable.h"
#include "MoveGenerator.h"
#include "PVTable.h"
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
  using SearchResult = std::optional<Eval>;

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
      : current_position_(position),
        move_generator_(MoveGenerator()),
        quiescence_searcher_() {}

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
  using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
  template <bool is_principal_variation>
  [[nodiscard]] SearchResult Search(const TimePoint &end_time, size_t max_depth,
                                    size_t remaining_depth, Eval alpha,
                                    Eval beta);

  [[nodiscard]] const DebugInfo &GetInfo() const { return debug_info_; }

  [[nodiscard]] std::size_t GetSearchedNodes() const {
    return debug_info_.searched_nodes;
  }

  [[nodiscard]] std::size_t GetPVHits() const { return debug_info_.pv_hits; }

  [[nodiscard]] const PVTable &GetPV() const { return principle_variation_; }

  void InitStartOfSearch();

 private:

  template <bool is_principal_variation>
  struct SearchImplementation {
   public:
    constexpr static size_t kEnoughNodesToCheckTime = 1 << 12;

    SearchImplementation(Searcher &searcher, const size_t max_depth,
                         const size_t remaining_depth, const Eval alpha,
                         const Eval beta, const TimePoint &end_time)
        : max_depth(max_depth),
          remaining_depth(remaining_depth),
          alpha(alpha),
          beta(beta),
          end_time(end_time),
          irreversible_data(searcher.current_position_.GetIrreversibleData()),
          side_to_move_idx(
              static_cast<size_t>(searcher.current_position_.GetSideToMove())),
          searcher_(searcher) {}

    template <bool is_principal_variation_search>
    SearchResult Search(const size_t max_depth, const size_t remaining_depth,
                        Eval alpha, const Eval beta) {
      return SearchImplementation<is_principal_variation_search>{
          searcher_, max_depth, remaining_depth, alpha, beta, end_time}();
    }

    bool IsTimeToExit() {
      return searcher_.debug_info_.searched_nodes % kEnoughNodesToCheckTime ==
                 0 &&
             std::chrono::high_resolution_clock::now() > end_time;
    }

    SearchResult operator()() {
      if (IsTimeToExit()) {
        return std::nullopt;
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

      if constexpr (is_principal_variation) {
        const auto &eval_optional = CheckPrincipalVariationMove();

        if (!eval_optional) return std::nullopt;

        best_eval = *eval_optional;

        if (best_eval > alpha) {
          if (best_eval >= beta) {
            return beta;
          }

          alpha = best_eval;
        }
      }

      auto &move_generator = searcher_.move_generator_;
      auto &current_position = searcher_.current_position_;

      auto moves = move_generator.GenerateMoves<MoveGenerator::Type::kDefault>(
          current_position);

      // check if there are no possible moves
      if (moves.empty()) {
        searcher_.principle_variation_.EndPV(max_depth - remaining_depth);
        return GetEndGameScore();
      }

      const auto ordering_result = OrderMoves(moves.begin(), moves.end());

      if constexpr (!is_principal_variation) {
        const auto eval_optional = CheckMove<false>(moves.front());

        if (!eval_optional) return std::nullopt;

        best_eval = *eval_optional;

        if (best_eval > alpha) {
          if (best_eval >= beta) {
            if (ordering_result.quiet_begin == moves.begin()) 
            {
              UpdateQuietMove(moves.front());
            }
            return beta;
          }

          alpha = best_eval;
        }
        SetBestMove(moves.front());
      }

      return PVSearch(std::next(moves.begin()), moves.end(), ordering_result);
    }

    /* Search args */
    const size_t max_depth;
    const size_t remaining_depth;
    Eval alpha;
    const Eval beta;
    const TimePoint &end_time;

    /* Local variables for search */
    Eval best_eval;
    const Position::IrreversibleData irreversible_data;
    const size_t side_to_move_idx;

   private:
    Eval QuiescenceSearch() {
      auto &current_position = searcher_.current_position_;
      auto &quiescence_searcher = searcher_.quiescence_searcher_;

      const auto eval =
          quiescence_searcher.Search<true>(current_position, alpha, beta);
      searcher_.debug_info_.quiescence_nodes +=
          quiescence_searcher.GetSearchedNodes();
      return eval;
    }

    Eval GetEndGameScore() const {
      if (searcher_.current_position_.IsUnderCheck()) {
        return kMateValue + static_cast<Eval>(max_depth - remaining_depth);
      }
      return Eval{};
    }

    void SetBestMove(const Move &move) {
      auto &current_position = searcher_.current_position_;

      searcher_.best_moves_.SetMove(searcher_.current_position_, move);

      auto &principle_variation = searcher_.principle_variation_;

      principle_variation.SetPV(move, max_depth - remaining_depth);
      principle_variation.FetchNextLayer(max_depth - remaining_depth,
                                         remaining_depth);

      if (remaining_depth == max_depth) {
        searcher_.best_move_ = move;
      }
    }

    SearchResult CheckPrincipalVariationMove()
      requires(is_principal_variation)
    {
      auto &current_position = searcher_.current_position_;
      auto &principle_variation = searcher_.principle_variation_;
      auto &debug_info = searcher_.debug_info_;

      if (!principle_variation.CheckPV(max_depth - remaining_depth))
        return GetEndGameScore();

      debug_info.pv_hits++;

      const auto &pv_move =
          principle_variation.GetPV(max_depth - remaining_depth);

      auto eval = CheckMove<true>(pv_move);
      SetBestMove(pv_move);
      return eval;
    }

    template <bool is_pv_move>
    SearchResult CheckMove(const Move &move) {
      auto &current_position = searcher_.current_position_;

      // make the move and search the tree
      current_position.DoMove(move);
      const auto eval_optional =
          Search<is_pv_move>(max_depth, remaining_depth - 1, -beta, -alpha);
      if (!eval_optional) return std::nullopt;

      const auto &eval = -*eval_optional;

      // undo the move
      current_position.UndoMove(move, irreversible_data);

      return eval;
    }

    struct QuietRange {
      MoveGenerator::Moves::iterator quiet_begin;
      MoveGenerator::Moves::iterator quiet_end;
    };

    QuietRange OrderMoves(const MoveGenerator::Moves::iterator first,
                              const MoveGenerator::Moves::iterator last) {
      auto &current_position = searcher_.current_position_;
      auto &best_moves = searcher_.best_moves_;
      auto &principal_variation = searcher_.principle_variation_;

      auto begin_of_ordering = first;

      if constexpr (is_principal_variation) {
        const auto pv_move_pos =
            std::find(first, last,
                      principal_variation.GetPV(max_depth - remaining_depth));
        std::iter_swap(begin_of_ordering, pv_move_pos);
        ++begin_of_ordering;
      } else {
        if (best_moves.Contains(current_position)) {
          const auto &best_move = best_moves.GetMove(current_position);
          const auto best_move_pos =
              std::find(begin_of_ordering, last, best_move);
          if (best_move_pos < last) {
            std::iter_swap(begin_of_ordering, best_move_pos);
            ++begin_of_ordering;
          }
        }
      }

      auto [quiet_begin, quiet_end] = searcher_.OrderMoves(
          begin_of_ordering, last, max_depth - remaining_depth,
          current_position.GetSideToMove());
      return QuietRange{quiet_begin, quiet_end};
    }

    SearchResult PVSearch(const MoveGenerator::Moves::iterator first,
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
          temp_eval_optional =
              Search<false>(max_depth, remaining_depth - 1, -beta, -alpha);
          if (!temp_eval_optional) return std::nullopt;

          temp_eval = -*temp_eval_optional;

          if (temp_eval > alpha) {
            alpha = temp_eval;
          }
        }

        // undo the move
        current_position.UndoMove(move, irreversible_data);

        if (temp_eval > best_eval) {
          SetBestMove(move);

          // check if we have found a better move
          if (temp_eval >= beta) {
            if (is_quiet) {
              UpdateQuietMove(move);
            }
            return beta;
          }

          best_eval = temp_eval;
        }
      }

      // return the best evaluation
      return alpha;
    }

    void UpdateQuietMove(const Move &move) {
      const auto [from, to, captured_piece] = GetMoveData(move);
      searcher_.history_[side_to_move_idx][from][to] +=
          1ull << remaining_depth;
      searcher_.killers_.TryAdd(max_depth - remaining_depth, move);
    }

    Searcher &searcher_;
  };

  std::pair<MoveGenerator::Moves::iterator, MoveGenerator::Moves::iterator>
  OrderMoves(const MoveGenerator::Moves::iterator first,
             const MoveGenerator::Moves::iterator last, const size_t ply,
             const Player color) const;
  Move best_move_;

  Position current_position_;  //!< Current position.

  MoveGenerator move_generator_;  //!< Move generator.
  Quiescence quiescence_searcher_;

  PVTable principle_variation_;  //!< PV table to store principal variation from
                                 //!< the previous iteration of ID

#ifdef _DEBUG
  constexpr static size_t kTTsize = 1 << 10;
#else
  constexpr static size_t kTTsize = 1 << 25;
#endif
  TranspositionTable<kTTsize>
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

void Searcher::InitStartOfSearch() {
  debug_info_ = DebugInfo{};
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
Searcher::SearchResult Searcher::Search(const TimePoint &end_time,
                                        const size_t max_depth,
                                        const size_t remaining_depth,
                                        Eval alpha, const Eval beta) {
  return SearchImplementation<is_principal_variation>{
      *this, max_depth, remaining_depth, alpha, beta, end_time}();
}

std::pair<MoveGenerator::Moves::iterator, MoveGenerator::Moves::iterator>
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
}  // namespace SimpleChessEngine
