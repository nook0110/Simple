#include <gtest/gtest.h>

#include <functional>
#include <memory>

#include "ExitCondition.h"
#include "MoveFactory.h"
#include "PositionFactory.h"
#include "Searcher.h"
#include "Settings.h"

using namespace SimpleChessEngine;

namespace {
class NullMovePruning : public testing::Test {
 protected:
  NullMovePruning() { rfp_.depth_limit = 0; }
  ~NullMovePruning() override { rfp_.depth_limit = rfp_depth_limit_; }

 private:
  Settings::PruneParameters::RFPSettings &rfp_ =
      Settings::current<Settings::PruneParameters::RFPSettings>;
  const Depth rfp_depth_limit_ = rfp_.depth_limit;
};

struct SearchTestCondition {
  const std::function<bool()> should_stop;

  bool IsTimeToExit() const { return should_stop(); }
};

SearchTestCondition StopAfterNodes(const Searcher &searcher,
                                   std::size_t limit) {
  return {[&searcher, limit] {
    return searcher.GetInfo().searched_nodes >= limit;
  }};
}

SearchTestCondition ObserveVerification(const Searcher &searcher,
                                        bool &observed,
                                        std::function<void()> observe) {
  return {[&searcher, &observed, observe = std::move(observe)] {
    if (!observed && searcher.GetNullMoveInfo().verifications != 0) {
      observed = true;
      observe();
      return true;
    }
    return false;
  }};
}

void StoreNullBound(Searcher::SearcherTranspositionTable &table,
                    Position position, Eval beta, Depth depth) {
  position.DoMove(NullMove{});
  table.SetEntry(position, MoveFactory{}(position, "e7e5"), ToTT(-beta, 0),
                 depth, Bound::kUpper, 0);
}
}

namespace SimpleChessEngine {
template <>
Quiescence<SearchTestCondition>::Quiescence(
    const SearchTestCondition &exit_condition, Table *table, Age age)
    : exit_condition_(exit_condition), table_(table), age_(age),
      cache_static_eval_(false), eval_context_(0) {}

template <>
template <>
SearchResult Quiescence<SearchTestCondition>::Search<true>(Position &, Eval,
                                                           Eval, Depth, bool) {
  ADD_FAILURE() << "Main-search probe unexpectedly reached quiescence";
  return std::nullopt;
}
}

TEST_F(NullMovePruning, RequiresConvertedDepthAndImprovingMargin) {
  const auto position = PositionFactory{}();
  struct Case {
    Depth depth;
    Eval gap;
    bool eligible;
    bool improving = false;
  };
  for (const auto test :
       {Case{1, 138, false},       Case{1, 139, true},
        Case{6, 113, false},       Case{6, 114, true},
        Case{12, 82, false},       Case{12, 83, true},
        Case{24, 20, false},       Case{24, 21, true},
        Case{28, 0, false},        Case{28, 1, true},
        Case{29, -5, false},       Case{29, -4, true},
        Case{32, -21, false},      Case{32, -20, true},
        Case{6, 94, false, true},  Case{6, 95, true, true},
        Case{12, 63, false, true}, Case{12, 64, true, true},
        Case{24, 2, false, true},  Case{24, 3, true, true},
        Case{25, -3, false, true}, Case{25, -2, true, true}}) {
    SCOPED_TRACE(testing::Message()
                 << "depth=" << +test.depth << " gap=" << test.gap);
    Searcher searcher(position);
    if (test.improving) {
      const auto ancestor = PositionFactory{}(
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
      ASSERT_LT(ancestor.Evaluate(), position.Evaluate());
      searcher.SetPosition(ancestor);
      const auto seed_stop = StopAfterNodes(searcher, 1);
      EXPECT_FALSE((SearchNode<NodeType::kAll, SearchTestCondition>{
          searcher, {32, 6, -1, 0, 1}, seed_stop}()));
      searcher.SetPosition(position);
    }
    const auto beta = position.Evaluate() - test.gap;
    const auto stop =
        StopAfterNodes(searcher, searcher.GetInfo().searched_nodes + 1);
    const auto result = SearchNode<NodeType::kCut, SearchTestCondition>{
        searcher, {32, test.depth, beta - 1, beta, 3}, stop}();
    EXPECT_FALSE(result);
    EXPECT_EQ(searcher.GetInfo().nmp_tries, test.eligible ? 1u : 0u);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
  }
}

TEST_F(NullMovePruning,
       RejectsNullPredecessorExclusionCheckPawnEndgameAndLoss) {
  struct Case {
    const char *fen;
    bool previous_null = false;
    bool excluded = false;
    bool loss_beta = false;
  };
  constexpr auto start =
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  for (const auto test : {Case{start, true}, Case{start, false, true},
                          Case{start, false, false, true},
                          Case{"4k3/8/8/8/8/8/4r3/4K1N1 w - - 0 1"},
                          Case{"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"}}) {
    const auto position = PositionFactory{}(test.fen);
    Searcher searcher(position);
    const auto beta = test.loss_beta ? -789 : position.Evaluate() - 200;
    const auto excluded = test.excluded
                              ? std::optional{MoveFactory{}(position, "e2e4")}
                              : std::nullopt;
    const auto stop = StopAfterNodes(searcher, 1);
    const auto result = SearchNode<NodeType::kCut, SearchTestCondition>{
        searcher,
        {12, 6, beta - 1, beta, 1, test.previous_null},
        stop,
        excluded}();
    EXPECT_FALSE(result);
    EXPECT_EQ(searcher.GetInfo().nmp_tries, 0u);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
  }
}

TEST_F(NullMovePruning, RequiresCutNode) {
  const auto position = PositionFactory{}();
  const auto beta = position.Evaluate() - 200;
  Searcher searcher(position);
  const auto stop = StopAfterNodes(searcher, 1);
  EXPECT_FALSE((SearchNode<NodeType::kPV, SearchTestCondition>{
      searcher, {12, 6, beta - 1, beta, 1}, stop}()));
  Searcher all_searcher(position);
  const auto all_stop = StopAfterNodes(all_searcher, 1);
  EXPECT_FALSE((SearchNode<NodeType::kAll, SearchTestCondition>{
      all_searcher, {12, 6, beta - 1, beta, 1}, all_stop}()));
  EXPECT_EQ(searcher.GetInfo().nmp_tries, 0u);
  EXPECT_EQ(all_searcher.GetInfo().nmp_tries, 0u);
}

TEST_F(NullMovePruning, AcceptsConvertedMinimumBetaInclusively) {
  const auto position = PositionFactory{}();
  for (const Eval beta : {-789, -788}) {
    Searcher searcher(position);
    const auto stop = StopAfterNodes(searcher, 1);
    EXPECT_FALSE((SearchNode<NodeType::kCut, SearchTestCondition>{
        searcher, {12, 6, beta - 1, beta, 1}, stop}()));
    EXPECT_EQ(searcher.GetInfo().nmp_tries, beta == -788 ? 1u : 0u);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
  }
}

TEST_F(NullMovePruning, StaticEvalAtBetaSetsImprovingForLaterPruning) {
  auto position = PositionFactory{}();
  const auto moves =
      MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(position);
  ASSERT_EQ(moves.size(), 20u);
  for (const Eval gap : {0, -1}) {
    const auto beta = position.Evaluate() - gap;
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    for (const auto &move : moves) {
      auto child = position;
      child.DoMove(move);
      table->SetEntry(child, MoveFactory{}(child, "e7e5"), ToTT(-beta + 1, 0),
                      1, Bound::kLower, 0);
    }
    Searcher searcher(position, table);
    const auto stop = StopAfterNodes(searcher, 21);
    const auto result = SearchNode<NodeType::kAll, SearchTestCondition>{
        searcher, {2, 2, beta - 1, beta, 1}, stop}();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, beta - 1);
    EXPECT_EQ(searcher.GetInfo().searched_nodes, gap == 0 ? 7u : 3u);
    EXPECT_EQ(searcher.GetInfo().lmp_prunes, gap == 0 ? 14u : 18u);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
  }
}

TEST_F(NullMovePruning, AdaptiveReductionUsesDepthMinusR) {
  const auto position = PositionFactory{}();
  struct Case {
    Depth depth;
    Eval gap;
    Depth child_depth;
  };
  for (const auto test : {Case{12, 100, 1}, Case{15, 100, 3}, Case{15, 101, 2},
                          Case{15, 201, 2}, Case{15, 202, 1}, Case{24, 202, 7},
                          Case{32, 707, 8}, Case{32, -20, 15}}) {
    SCOPED_TRACE(testing::Message()
                 << "depth=" << +test.depth << " gap=" << test.gap);
    const auto beta = position.Evaluate() - test.gap;
    for (const bool sufficient_depth : {true, false}) {
      auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
      StoreNullBound(*table, position, beta,
                     static_cast<Depth>(test.child_depth - !sufficient_depth));
      table->SetEntry(position, MoveFactory{}(position, "e2e4"), ToTT(beta, 0),
                      test.child_depth, Bound::kLower, 0);
      Searcher searcher(position, table);
      const auto stop = StopAfterNodes(searcher, sufficient_depth ? 3u : 2u);
      const auto result = SearchNode<NodeType::kCut, SearchTestCondition>{
          searcher, {24, test.depth, beta - 1, beta, 1}, stop}();
      if (sufficient_depth) {
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, beta);
        EXPECT_EQ(searcher.GetInfo().nmp_cuts, 1u);
        EXPECT_EQ(searcher.GetInfo().searched_nodes,
                  test.depth >= 16 ? 3u : 2u);
      } else {
        EXPECT_FALSE(result);
        EXPECT_EQ(searcher.GetInfo().nmp_cuts, 0u);
      }
      EXPECT_EQ(searcher.GetInfo().nmp_tries, 1u);
      EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
    }
  }
}

TEST_F(NullMovePruning, ShallowReductionReachesQuiescenceWithoutWrapping) {
  const auto position = PositionFactory{}();
  DepthCondition stop{12};
  for (const Depth depth : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}) {
    Searcher searcher(position);
    const auto beta = position.Evaluate() - 200;
    const auto result = SearchNode<NodeType::kCut, DepthCondition>{
        searcher, {12, depth, beta - 1, beta, 1}, stop}();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, beta);
    EXPECT_EQ(searcher.GetInfo().searched_nodes, 1u);
    EXPECT_GT(searcher.GetInfo().quiescence_nodes, 0u);
    EXPECT_EQ(searcher.GetInfo().nmp_tries, 1u);
    EXPECT_EQ(searcher.GetInfo().nmp_cuts, 1u);
    EXPECT_EQ(searcher.GetNullMoveInfo().verifications, 0u);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
  }
}

TEST_F(NullMovePruning, UncappedReductionCanVerifyInQuiescence) {
  const auto position = PositionFactory{}();
  Searcher searcher(position);
  const auto beta = position.Evaluate() - 707;
  ASSERT_GE(beta, -788);
  auto budget = std::make_shared<NodeBudget>(4);
  NodeCondition underlying_condition{16, {}, budget};
  NodeReportingCondition stop{underlying_condition};
  const auto result = SearchNode<NodeType::kCut, decltype(stop)>{
      searcher, {16, 16, beta - 1, beta, 1}, stop}();
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, beta);
  EXPECT_EQ(searcher.GetInfo().searched_nodes, 1u);
  EXPECT_GE(searcher.GetInfo().quiescence_nodes, 2u);
  EXPECT_EQ(searcher.GetInfo().nmp_tries, 1u);
  EXPECT_EQ(searcher.GetInfo().nmp_cuts, 1u);
  EXPECT_EQ(searcher.GetNullMoveInfo().verifications, 1u);
  EXPECT_EQ(searcher.GetNodes(), searcher.GetInfo().searched_nodes +
                                    searcher.GetInfo().quiescence_nodes);
  EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
  EXPECT_FALSE(budget->IsSpent());
  budget->Spend();
  EXPECT_TRUE(budget->IsSpent());
}

TEST_F(NullMovePruning, VerificationMustConfirmTheNullCutoff) {
  const auto position = PositionFactory{}();
  const auto beta = position.Evaluate() - 101;
  for (const bool confirmed : {true, false}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    StoreNullBound(*table, position, beta, 3);
    table->SetEntry(position, MoveFactory{}(position, "e2e4"),
                    ToTT(confirmed ? beta : beta - 1, 0), 3,
                    confirmed ? Bound::kLower : Bound::kUpper, 0);
    Searcher searcher(position, table);
    const auto stop = StopAfterNodes(searcher, 3);
    const auto result = SearchNode<NodeType::kCut, SearchTestCondition>{
        searcher, {16, 16, beta - 1, beta, 1}, stop}();
    EXPECT_EQ(result.has_value(), confirmed);
    if (result) {
      EXPECT_EQ(*result, beta);
    }
    EXPECT_EQ(searcher.GetInfo().nmp_tries, 1u);
    EXPECT_EQ(searcher.GetInfo().nmp_cuts, confirmed ? 1u : 0u);
    EXPECT_EQ(searcher.GetNullMoveInfo().verifications, 1u);
    EXPECT_EQ(searcher.GetNullMoveInfo().verification_fail_lows,
              confirmed ? 0u : 1u);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
  }
}

TEST_F(NullMovePruning, VerificationPlyLimitIsLocalNonrecursiveAndRestored) {
  const auto position = PositionFactory{}();
  const auto beta = position.Evaluate() - 101;
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  StoreNullBound(*table, position, beta, 3);
  table->SetEntry(position, MoveFactory{}(position, "e2e4"), ToTT(beta, 0), 3,
                  Bound::kLower, 0);
  Searcher searcher(position, table);
  bool observed = false;
  const auto stop = ObserveVerification(searcher, observed, [&] {
    for (const Depth ply : {2, 3}) {
      const auto tries = searcher.GetInfo().nmp_tries;
      const auto probe_beta = position.Evaluate() - 120;
      const auto probe_stop =
          StopAfterNodes(searcher, searcher.GetInfo().searched_nodes + 1);
      EXPECT_FALSE((SearchNode<NodeType::kCut, SearchTestCondition>{
          searcher, {16, 6, probe_beta - 1, probe_beta, ply}, probe_stop}()));
      EXPECT_EQ(searcher.GetInfo().nmp_tries - tries, ply == 3 ? 1u : 0u);
    }
    const auto nested_stop =
        StopAfterNodes(searcher, searcher.GetInfo().searched_nodes + 2);
    const auto nested = SearchNode<NodeType::kCut, SearchTestCondition>{
        searcher, {16, 16, beta - 1, beta, 3}, nested_stop}();
    ASSERT_TRUE(nested);
    EXPECT_EQ(*nested, beta);
    EXPECT_EQ(searcher.GetNullMoveInfo().verifications, 1u);

    Searcher other(position);
    const auto other_stop = StopAfterNodes(other, 1);
    EXPECT_FALSE((SearchNode<NodeType::kCut, SearchTestCondition>{
        other, {16, 6, beta - 121, beta - 120, 1}, other_stop}()));
    EXPECT_EQ(other.GetInfo().nmp_tries, 1u);
  });
  EXPECT_FALSE((SearchNode<NodeType::kCut, SearchTestCondition>{
      searcher, {16, 16, beta - 1, beta, 1}, stop}()));
  EXPECT_TRUE(observed);
  const auto retry_stop =
      StopAfterNodes(searcher, searcher.GetInfo().searched_nodes + 3);
  const auto retry = SearchNode<NodeType::kCut, SearchTestCondition>{
      searcher, {16, 16, beta - 1, beta, 1}, retry_stop}();
  ASSERT_TRUE(retry);
  EXPECT_EQ(*retry, beta);
  EXPECT_EQ(searcher.GetNullMoveInfo().verifications, 2u);
  EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
}

TEST_F(NullMovePruning, DoesNotAcceptUnprovenTablebaseWins) {
  const auto tablebase = Tablebase::Syzygy::Open("syzygy-fixture");
  ASSERT_TRUE(tablebase);
  struct TablebaseGuard {
    ~TablebaseGuard() { Tablebase::Syzygy::Disable(); }
  } guard;
  const auto position = PositionFactory{}("7k/8/6K1/8/8/8/8/1Q6 w - - 0 1");
  const auto beta = position.Evaluate() - 101;
  auto null_position = position;
  null_position.DoMove(NullMove{});
  ASSERT_EQ(tablebase->ProbeWdl(null_position), Tablebase::Wdl::kLoss);
  Searcher searcher(position,
                    std::make_shared<Searcher::SearcherTranspositionTable>(),
                    tablebase);
  const SearchTestCondition stop{[&] {
    return searcher.GetInfo().nmp_tries != 0 &&
           searcher.GetPosition().GetHash() != null_position.GetHash();
  }};
  EXPECT_FALSE((SearchNode<NodeType::kCut, SearchTestCondition>{
      searcher, {24, 24, beta - 1, beta}, stop}()));
  EXPECT_EQ(searcher.GetInfo().nmp_tries, 1u);
  EXPECT_EQ(searcher.GetInfo().nmp_cuts, 0u);
  EXPECT_EQ(searcher.GetNullMoveInfo().verifications, 0u);
  EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
}
