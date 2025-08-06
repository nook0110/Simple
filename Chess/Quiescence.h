#pragma once
#include "Concepts.h"
#include "Evaluation.h"
#include "MoveGenerator.h"

namespace SimpleChessEngine {
class Position;

template <class ExitCondition>
  requires StopSearchCondition<ExitCondition>
class Quiescence {
 public:
  constexpr static size_t kEnoughNodesToCheckTime = 1 << 12;
  constexpr static Eval kSEEMargin = 120;

  /**
   * \brief Constructor.
   *
   */
  Quiescence(const ExitCondition& exit_condition)
      : exit_condition_(exit_condition) {};

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
                                    Eval beta, const Depth current_depth);

  [[nodiscard]] SearchResult SearchUnderCheck(Position& current_position,
                                              Eval alpha, Eval beta,
                                              const Depth current_depth);

  [[nodiscard]] std::size_t GetSearchedNodes() const { return searched_nodes_; }

 private:
  bool IsTimeToExit();

  MoveGenerator move_generator_;  //!< Move generator.
  const ExitCondition& exit_condition_;
  std::size_t searched_nodes_{};
};

}  // namespace SimpleChessEngine