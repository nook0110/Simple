#pragma once
#include <array>
#include <cstddef>

#include "Concepts.h"
#include "DebugInfo.h"
#include "Evaluation.h"
#include "KillerTable.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "NodeType.h"
#include "PositionFactory.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine {
template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
struct SearchNode;

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
  template <NodeType node_type, class ExitCondition>
    requires StopSearchCondition<ExitCondition>
  friend struct SearchNode;
  constexpr static size_t kTTsize = 1 << 26;
  using SearcherTranspositionTable = TranspositionTable<kTTsize>;

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
  template <NodeType node_type, class ExitCondition>
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
  SearcherTranspositionTable
      best_moves_;  //!< Transposition-table to store the best moves.
  std::array<
      std::array<std::array<std::int64_t, kBoardArea + 1>, kBoardArea + 1>,
      kColors>
      history_ = {};
  KillerTable<2> killers_;

  DebugInfo debug_info_;
};

}  // namespace SimpleChessEngine

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////
#include "SearchImplementation.h"

namespace SimpleChessEngine {
inline void Searcher::SetPosition(Position position) {
  current_position_ = std::move(position);
}

inline const Position &Searcher::GetPosition() const { return current_position_; }

inline const Move &Searcher::GetCurrentBestMove() const { return best_move_; }

inline MoveGenerator::Moves Searcher::GetPrincipalVariation(Depth max_depth,
                                                     Position position) const {
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
    for (BitIndex from = 0; from <= static_cast<BitIndex>(kBoardArea); ++from) {
      history_[color][from].fill(0LL);
    }
  }
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SimpleChessEngine::Searcher::Search(
    const ExitCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta) {
  debug_info_ = DebugInfo{};
  ++age_;

  return SearchNode<node_type, ExitCondition>{
      *this,
      {max_depth, remaining_depth, alpha, beta},
      stop_search_condition}();
}
}  // namespace SimpleChessEngine
