#pragma once
#include "Concepts.h"
#include "Evaluation.h"
#include "MoveGenerator.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine {
class Position;

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
class Quiescence {
 public:
  constexpr static Depth kMaxDepth = kMaxSearchPly + 32;
  constexpr static Eval kSEEMargin = 120;

  using Table = TranspositionTable<SCE_TT_SIZE_MB>;
  static constexpr Depth kTTDepth = 0;

  /**
   * \brief Constructor.
   *
   */
  Quiescence(const ExitCondition& exit_condition, Table* table = nullptr,
             Age age = 0);

  /**
   * \brief Performs the alpha-beta search algorithm.
   *
   * \param current_position Current position.
   * \param alpha The current alpha value.
   * \param beta The current beta value.
   *
   * \return Evaluation of subtree.
   */
  template <bool start_of_search>
  [[nodiscard]] SearchResult Search(Position& current_position, Eval alpha,
                                    Eval beta, const Depth current_depth,
                                    bool pv_node = true);

  [[nodiscard]] std::size_t GetSearchedNodes() const { return searched_nodes_; }

 private:
  bool IsTimeToExit();
  [[nodiscard]] SearchResult SearchUnderCheck(Position& current_position,
                                              Eval alpha, Eval beta,
                                              Depth current_depth, bool pv_node,
                                              Move tt_move, Move& best_move);
  void Store(const Position& position, Move move, Eval value, Bound bound,
             Depth ply, Eval static_eval);

  MoveGenerator move_generator_;  //!< Move generator.
  const ExitCondition& exit_condition_;
  Table* table_;
  Age age_;
  bool cache_static_eval_;
  std::uint64_t eval_context_;
  std::size_t searched_nodes_{};
};

}  // namespace SimpleChessEngine