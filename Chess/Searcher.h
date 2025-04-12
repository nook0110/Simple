#pragma once
#include <glog/logging.h>

#include <array>
#include <cstddef>

#include "Concepts.h"
#include "DebugInfo.h"
#include "Evaluation.h"
#include "KillerTable.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "MovePicker.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine {
template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
struct SearchImplementation;

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
  template <bool is_principal_variation, class ExitCondition>
    requires StopSearchCondition<ExitCondition>
  friend struct SearchImplementation;
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
