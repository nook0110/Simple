#include "Searcher.h"

#include "ExitCondition.h"
#include "SearchImplementation.h"

namespace SimpleChessEngine {
void Searcher::SetPosition(Position position) {
  current_position_ = std::move(position);
}

const Position &Searcher::GetPosition() const { return current_position_; }

const Move &Searcher::GetCurrentBestMove() const { return best_move_; }

MoveGenerator::Moves Searcher::GetPrincipalVariation(Depth max_depth,
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

void Searcher::InitStartOfSearch() {
  killers_.Clear();
  for (size_t color = 0; color < kColors; ++color) {
    for (BitIndex from = 0; from <= kBoardArea; ++from) {
      history_[color][from].fill(0ull);
    }
  }
}

template <bool is_principal_variation, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SimpleChessEngine::Searcher::Search(
    const ExitCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta) {
  debug_info_ = DebugInfo{};
  ++age_;

  return SearchImplementation<is_principal_variation, ExitCondition>{
      *this,
      {max_depth, remaining_depth, alpha, beta},
      stop_search_condition}();
}

template SearchResult SimpleChessEngine::Searcher::Search<false, TimeCondition>(
    const TimeCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta);

template SearchResult SimpleChessEngine::Searcher::Search<true, TimeCondition>(
    const TimeCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta);

template SearchResult
SimpleChessEngine::Searcher::Search<false, DepthCondition>(
    const DepthCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta);

template SearchResult SimpleChessEngine::Searcher::Search<true, DepthCondition>(
    const DepthCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta);

template SearchResult SimpleChessEngine::Searcher::Search<false, Pondering>(
    const Pondering &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta);

template SearchResult SimpleChessEngine::Searcher::Search<true, Pondering>(
    const Pondering &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta);
}  // namespace SimpleChessEngine
