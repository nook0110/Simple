#pragma once
#include <algorithm>

#include "Evaluation.h"
#include "KillerTable.h"
#include "MoveGenerator.h"
#include "PVTable.h"
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
 *  - Aspiration windows
 *  - Transposition table
 *  - Iterative deepening
 *  - Quiescence search
 *
 *  \author nook0110
 */
class Searcher {
 public:
  struct DebugInfo {
    std::size_t searched_nodes{};
    std::size_t quiescence_nodes{};
    std::size_t pv_hits{};

    DebugInfo& operator+=(const DebugInfo& other) {
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
  void SetPosition(const Position& position);

  /**
   * \brief Returns the current position.
   *
   * \return The current position.
   */
  [[nodiscard]] const Position& GetPosition() const;

  /**
   * \brief Returns the best move for the current position.
   *
   * \return The current best move.
   */
  [[nodiscard]] const Move& GetCurrentBestMove() const;

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
  template <bool is_pv>
  [[nodiscard]] Eval Search(size_t max_depth, size_t remaining_depth,
                            Eval alpha, Eval beta);

  [[nodiscard]] const DebugInfo& GetInfo() const { return debug_info_; }

  [[nodiscard]] std::size_t GetSearchedNodes() const {
    return debug_info_.searched_nodes;
  }

  [[nodiscard]] std::size_t GetPVHits() const { return debug_info_.pv_hits; }

  [[nodiscard]] const PVTable& GetPV() const { return principle_variation_; }

 private:
  std::pair<MoveGenerator::Moves::iterator, MoveGenerator::Moves::iterator>
  OrderMoves(MoveGenerator::Moves::iterator first,
             MoveGenerator::Moves::iterator last, const size_t ply, const size_t color_idx) const {
    MoveGenerator::Moves::iterator quiet_begin;
    quiet_begin = std::partition(first, last, [this](const Move& move) {
      if (std::holds_alternative<Promotion>(move) ||
          std::holds_alternative<EnCroissant>(move))
        return true;
      if (!std::holds_alternative<DefaultMove>(move)) return false;
      const auto [from, to, captured_piece] = std::get<DefaultMove>(move);
      return static_cast<size_t>(captured_piece) >=
             static_cast<size_t>(current_position_.GetPiece(from));
    });
    MoveGenerator::Moves::iterator quiet_end;
    quiet_end = std::partition(quiet_begin, last, [](const Move& move) {
      if (!std::holds_alternative<DefaultMove>(move)) return true;
      const auto [from, to, captured_piece] = std::get<DefaultMove>(move);
      return !captured_piece;
    });
    const auto CompareCaptures = [this](const Move& lhs, const Move& rhs) {
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
    for (size_t i = 0; i < killers_.AvailableKillerCount(ply); ++i) {
      const auto killer = killers_.Get(ply, i);
      if (auto it = std::find(quiet_begin, quiet_end, killer);
          it != quiet_end) {
        std::iter_swap(quiet_begin, it);
        ++quiet_begin;
      }
    }
    std::sort(quiet_begin, quiet_end, [this, color_idx](const Move& lhs, const Move& rhs) {
      const auto [from_lhs, to_lhs, captured_piece_lhs] = GetMoveData(lhs);
      const auto [from_rhs, to_rhs, captured_piece_rhs] = GetMoveData(rhs);
      return history_[color_idx][from_lhs][to_lhs] >
             history_[color_idx][from_rhs][to_rhs];
    });
    return {quiet_begin, quiet_end};
  }

  Move best_move_;

  Position current_position_;  //!< Current position.

  MoveGenerator move_generator_;  //!< Move generator.
  Quiescence quiescence_searcher_;

  PVTable principle_variation_;  //!< PV table to store principal variation from
                                 //!< the previous iteration of ID

  TranspositionTable<1 << 25>
      best_moves_;  //!< Transposition-table to store the best moves.

  std::array<std::array<std::array<uint64_t, kBoardArea + 1>, kBoardArea + 1>, kColors> history_;
  
  KillerTable<2> killers_;

  DebugInfo debug_info_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
inline void Searcher::SetPosition(const Position& position) {
  current_position_ = position;
}

inline const Position& Searcher::GetPosition() const {
  return current_position_;
}

inline const Move& Searcher::GetCurrentBestMove() const { return best_move_; }

template <bool is_pv>
Eval Searcher::Search(const size_t max_depth, const size_t remaining_depth,
                      Eval alpha, const Eval beta) {
  if constexpr (is_pv) {
    if (remaining_depth == 1) {
      return Search<false>(max_depth, remaining_depth, alpha, beta);
    }
  }

  const bool is_start_of_search = max_depth == remaining_depth;
  const auto color_idx = static_cast<size_t>(current_position_.GetSideToMove());

  if (is_start_of_search) {
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
  // return the evaluation of the current position if we have reached the
  // end of the search tree
  if (remaining_depth <= 0) {
    const auto eval =
        quiescence_searcher_.Search<true>(current_position_, alpha, beta);
    debug_info_.quiescence_nodes += quiescence_searcher_.GetSearchedNodes();
    return eval;
  }

  debug_info_.searched_nodes++;

  // lambda function that sets best move
  auto set_best_move = [this, is_start_of_search, max_depth,
                        remaining_depth](const Move& move) {
    best_moves_[current_position_] = {move, current_position_.GetHash()};

    if (is_start_of_search) {
      best_move_ = move;
    }

    principle_variation_.SetPV(move, remaining_depth, remaining_depth);
    principle_variation_.FetchNextLayer(remaining_depth);
  };

  // get all the possible moves
  auto moves = move_generator_.GenerateMoves<MoveGenerator::Type::kDefault>(
      current_position_);

  // check if there are no possible moves
  if (moves.empty()) {
    if (current_position_.IsUnderCheck()) {
      return kMateValue - static_cast<Eval>(remaining_depth);
    }
    return Eval{};
  }

  const Move* best_move{};

  if constexpr (is_pv) {
    best_move = &principle_variation_.GetPV(max_depth, remaining_depth);
  } else {
    // check if we have already searched this position
    if (best_moves_.Contains(current_position_)) {
      best_move = &best_moves_[current_position_].move;
    }
  }
  if (best_move) {
    // find the best move in moves
    if (const auto pv = std::ranges::find(moves, *best_move);
        pv != moves.end()) {
      debug_info_.pv_hits++;

      std::iter_swap(pv, moves.begin());
    }
  }

  auto [quiet_begin, quiet_end] =
      OrderMoves(moves.begin() + static_cast<bool>(best_move), moves.end(),
                 max_depth - remaining_depth, color_idx);

  const auto& first_move = moves.front();
  const auto irreversible_data = current_position_.GetIrreversibleData();
  // make the move and search the tree
  current_position_.DoMove(first_move);
  auto best_eval =
      -Search<is_pv>(max_depth, remaining_depth - 1, -beta, -alpha);
  // undo the move
  current_position_.UndoMove(first_move, irreversible_data);

  if (best_eval > alpha) {
    if (best_eval >= beta) {
      return best_eval;
    }

    alpha = best_eval;
  }

  // set best move
  set_best_move(moves.front());

  bool is_quiet = false;

  // search the tree
  for (auto it = std::next(moves.begin()); it != moves.end(); ++it) {
    const auto move = *it;

    if (it == quiet_begin) is_quiet = true;
    if (it == quiet_end) is_quiet = false;
    // make the move and search the tree
    current_position_.DoMove(move);

    // ZWS
    auto temp_eval =
        -Search<false>(max_depth, remaining_depth - 1, -alpha - 1, -alpha);

    // make a research (ZWS failed)
    if (temp_eval > alpha && temp_eval < beta) {
      temp_eval = -Search<false>(max_depth, remaining_depth - 1, -beta, -alpha);

      if (temp_eval > alpha) {
        alpha = temp_eval;
      }
    }

    // undo the move
    current_position_.UndoMove(move, irreversible_data);

    if (temp_eval > best_eval) {
      set_best_move(move);

      // check if we have found a better move
      if (temp_eval >= beta) {
        if (is_quiet) {
          const auto [from, to, captured_piece] = GetMoveData(move);
          history_[color_idx][from][to] += remaining_depth * remaining_depth;
          killers_.TryAdd(max_depth - remaining_depth, move);
        }
        return temp_eval;
      }

      best_eval = temp_eval;
    }
  }

  // return the best evaluation
  return best_eval;
}
}  // namespace SimpleChessEngine
