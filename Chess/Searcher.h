#pragma once
#include <algorithm>

#include "Evaluation.h"
#include "MoveGenerator.h"
#include "PVTable.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine
{
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
class Searcher
{
 public:
  struct DebugInfo
  {
    std::size_t searched_nodes{};
    std::size_t pv_hits{};

    DebugInfo& operator+=(const DebugInfo& other)
    {
      searched_nodes += other.searched_nodes;
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
        quiescence_searcher_()
  {}

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

  [[nodiscard]] const TranspositionTable& GetTranspositionTable() const;

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

  [[nodiscard]] std::size_t GetSearchedNodes() const
  {
    return debug_info_.searched_nodes;
  }

  [[nodiscard]] std::size_t GetPVHits() const { return debug_info_.pv_hits; }

  [[nodiscard]] const PVTable& GetPV() const { return principle_variation_; }

 private:
  Move best_move_;

  Position current_position_;  //!< Current position.

  MoveGenerator move_generator_;  //!< Move generator.
  Quiescence quiescence_searcher_;

  TranspositionTable
      best_moves_;  //!< Transposition-table to store the best moves.

  PVTable principle_variation_;  //!< PV table to store principal variation from
                                 //!< the previous iteration of ID

  DebugInfo debug_info_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine
{
inline void Searcher::SetPosition(const Position& position)
{
  current_position_ = position;
}

inline const Position& Searcher::GetPosition() const
{
  return current_position_;
}

inline const Move& Searcher::GetCurrentBestMove() const { return best_move_; }

inline const TranspositionTable& Searcher::GetTranspositionTable() const
{
  return best_moves_;
}

template <bool is_pv>
Eval Searcher::Search(const size_t max_depth, const size_t remaining_depth,
                      Eval alpha, const Eval beta)
{
  if constexpr (is_pv)
  {
    if (remaining_depth == 1)
    {
      return Search<false>(max_depth, remaining_depth, alpha, beta);
    }
  }

  const bool is_start_of_search = max_depth == remaining_depth;
  if (is_start_of_search)
  {
    debug_info_ = DebugInfo{};
  }
  // return the evaluation of the current position if we have reached the
  // end of the search tree
  if (remaining_depth <= 0)
  {
    const auto eval =
        quiescence_searcher_.Search<true>(current_position_, alpha, beta);
    debug_info_.searched_nodes += quiescence_searcher_.GetSearchedNodes();
    return eval;
  }

  debug_info_.searched_nodes++;

  // lambda function that sets best move
  auto set_best_move =
      [this, is_start_of_search, max_depth, remaining_depth](const Move& move)
  {
    best_moves_[current_position_] = {move, current_position_.GetHash()};

    if (is_start_of_search)
    {
      best_move_ = move;
    }
  };

  // get all the possible moves
  auto moves = move_generator_.GenerateMoves<MoveGenerator::Type::kDefault>(
      current_position_);

  // check if there are no possible moves
  if (moves.empty())
  {
    if (current_position_.IsUnderCheck())
    {
      return kMateValue - static_cast<Eval>(remaining_depth);
    }
    return Eval{};
  }

  const Move* best_move{};

  if constexpr (is_pv)
  {
    best_move = &principle_variation_.GetPV(max_depth, remaining_depth);
  }
  else
  {
    // check if we have already searched this position
    if (best_moves_.Contains(current_position_))
    {
      best_move = &best_moves_[current_position_].move;
    }
  }
  if (best_move)
  {
    // find the best move in moves
    if (const auto pv = std::ranges::find(moves, *best_move); pv != moves.end())
    {
      debug_info_.pv_hits++;

      std::iter_swap(pv, moves.begin());

      // sort all moves except first (PV-move)
      std::sort(std::next(moves.begin()), moves.end(), std::greater{});
    }
  }
  else
  {
    std::ranges::sort(moves, std::greater{});
  }

  const auto& first_move = moves.front();
  const auto irreversible_data = current_position_.GetIrreversibleData();
  // make the move and search the tree
  current_position_.DoMove(first_move);
  auto best_eval =
      -Search<is_pv>(max_depth, remaining_depth - 1, -beta, -alpha);
  // undo the move
  current_position_.UndoMove(first_move, irreversible_data);

  if (best_eval > alpha)
  {
    if (best_eval >= beta)
    {
      return best_eval;
    }

    alpha = best_eval;
    principle_variation_.SetPV(first_move, remaining_depth, remaining_depth);
    principle_variation_.FetchNextLayer(remaining_depth);
  }

  // set best move
  set_best_move(moves.front());

  // delete first move
  std::iter_swap(moves.begin(), std::prev(moves.end()));
  moves.pop_back();

  // search the tree
  for (const auto& move : moves)
  {
    // make the move and search the tree
    current_position_.DoMove(move);

    // ZWS
    auto temp_eval =
        -Search<false>(max_depth, remaining_depth - 1, -alpha - 1, -alpha);

    // make a research (ZWS failed)
    if (temp_eval > alpha && temp_eval < beta)
    {
      temp_eval = -Search<false>(max_depth, remaining_depth - 1, -beta, -alpha);

      if (temp_eval > alpha)
      {
        alpha = temp_eval;
      }
    }

    // undo the move
    current_position_.UndoMove(move, irreversible_data);

    if (temp_eval > best_eval)
    {
      principle_variation_.SetPV(move, remaining_depth, remaining_depth);
      principle_variation_.FetchNextLayer(remaining_depth);
      set_best_move(move);

      // check if we have found a better move
      if (temp_eval >= beta)
      {
        return temp_eval;
      }

      best_eval = temp_eval;
    }
  }

  // return the best evaluation
  return best_eval;
}
}  // namespace SimpleChessEngine
