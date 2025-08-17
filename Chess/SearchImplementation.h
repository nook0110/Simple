#pragma once

#include <cassert>

#include "Concepts.h"
#include "Evaluation.h"
#include "Move.h"
#include "MovePicker.h"
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

  bool was_previous_move_a_null = false;
};

struct IterationStatus {
  bool has_raised_alpha = false;
  std::optional<Move> best_move;
  std::optional<Node> tt_info;
  Eval best_eval = {};
};

struct PositionInfo {
  PositionInfo(const Position &position);
  const Eval static_eval;
  const bool is_under_check = false;
  const Position::IrreversibleData irreversible_data;
  const size_t side_to_move_idx;
};

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
struct SearchNode {
 public:
  constexpr static size_t kEnoughNodesToCheckTime = 1 << 12;

  SearchNode(Searcher &searcher, SearchState state,
             const ExitCondition &exit_condition);

  template <bool is_principal_variation_search>
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

  SearchResult QuiescenceSearch();
  Eval GetEndGameScore() const;

  void SetBestMove(Move move);
  void SetTTEntry(const Bound bound);
  template <bool is_first_move>
  void UpdateQuietMove(const Move &move);

  Position &GetCurrentPosition();

  template <bool is_pv_move>
  SearchResult ProbeMove(const Move &move);
  template <bool is_pv_move>
  std::optional<bool> CheckFirstMove(const Move &move);

  [[nodiscard]] bool CanRFP() const;

  bool ProbeTranspositionTable();
  std::optional<SearchResult> CheckTranspositionTable();

  SearchResult PVSearch();

  [[nodiscard]] bool CanNullMove() const;

  Searcher &searcher_;
};
}  // namespace SimpleChessEngine
