#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "ExitCondition.h"
#include "MoveFactory.h"
#include "Nnue/Nnue.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "Searcher.h"

using namespace SimpleChessEngine;

TEST(QuiescenceDraws, ExpiredFiftyMoveClock) {
  auto position = PositionFactory{}("7k/8/6K1/8/8/8/8/1Q6 w - - 100 51");
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kDrawValue);
}

TEST(QuiescenceDraws, RepetitionInGameHistory) {
  auto position = PositionFactory{}("7k/8/6K1/8/8/8/8/1Q6 w - - 0 1");
  for (const auto* move : {"b1b2", "h8g8", "b2b1", "g8h8", "b1b2",
                           "h8g8", "b2b1", "g8h8"}) {
    position.DoMove(MoveFactory{}(position, move));
  }
  ASSERT_TRUE(position.DetectRepetition(0));
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kDrawValue);
}

TEST(QuiescenceDraws, StalemateWithPinnedBishop) {
  auto position = PositionFactory{}("7k/5K1b/7Q/8/8/8/8/8 b - - 0 1");
  ASSERT_FALSE(position.IsUnderCheck());
  ASSERT_TRUE(MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position)
                  .empty());
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kDrawValue);
}

TEST(QuiescenceDraws, MateOverridesExpiredFiftyMoveClock) {
  auto position = PositionFactory{}("7k/6Q1/6K1/8/8/8/8/8 b - - 100 51");
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kMateValue + 1);
}

TEST(TranspositionTable, LaterMoveCutoffStoresCurrentScore) {
  const auto position = PositionFactory{}();
  DepthCondition condition{2};
  size_t cutoffs = 0;
  for (Eval beta = -100; beta <= 100; beta += 2) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    Searcher searcher(position, table);
    searcher.InitStartOfSearch();
    const auto result =
        searcher.Search<NodeType::kPV>(condition, 2, 2, beta - 1, beta);
    ASSERT_TRUE(result.has_value());
    const auto node = table->GetNode(position);
    if (node.IsOccupied() && node.bound == Bound::kLower) {
      ++cutoffs;
      EXPECT_GE(node.score, beta) << "beta=" << beta;
    }
  }
  EXPECT_GT(cutoffs, 0u);
}

TEST(SingularExtensions, ExclusionIgnoresBoundsAndPreservesFullNodeEntry) {
  const auto position = PositionFactory{}();
  const auto excluded = MoveFactory{}(position, "e2e4");
  DepthCondition condition{8};
  for (const auto bound : {Bound::kLower, Bound::kExact, Bound::kUpper}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    table->SetEntry(position, excluded, 10000, 3, bound, 0);
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kAll, DepthCondition>{
        searcher, {8, 3, 499, 500, 1}, condition, excluded}();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 499);
    EXPECT_GT(searcher.GetInfo().searched_nodes, 1u);
    const auto node = table->GetNode(position);
    EXPECT_EQ(node.move, excluded);
    EXPECT_EQ(node.score, 10000);
    EXPECT_EQ(node.depth, 3);
    EXPECT_EQ(node.bound, bound);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
    EXPECT_EQ(searcher.GetSingularInfo().searches, 0u);
  }
}

TEST(SingularExtensions, ExcludingOnlyEvasionReturnsFailLow) {
  auto position = PositionFactory{}("r7/8/8/8/8/2k5/8/K7 w - - 0 1");
  const auto moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
  ASSERT_EQ(moves.size(), 1u);
  ASSERT_TRUE(position.IsUnderCheck());
  Searcher searcher(position);
  DepthCondition condition{8};
  const auto result = SearchNode<NodeType::kAll, DepthCondition>{
      searcher, {8, 3, 499, 500, 1}, condition, moves.front()}();
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, 499);
}

TEST(SingularExtensions, ExtendsOnlyEligibleLowerBounds) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  DepthCondition condition{8};
  struct Case {
    Bound bound;
    Depth depth;
    Eval score;
    bool eligible;
  };
  for (const auto test : {
           Case{Bound::kLower, 4, 10000, true},
           Case{Bound::kExact, 4, 10000, true},
           Case{Bound::kUpper, 4, 10000, false},
           Case{Bound::kLower, 3, 10000, false},
           Case{Bound::kLower, 4, -kMateValue - 10, false},
           Case{Bound::kLower, 4, kTablebaseWinValue - 10, false}}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    table->SetEntry(position, move, test.score, test.depth, test.bound, 0);
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kPV, DepthCondition>{
        searcher, {8, 7, -20000, 20000, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(searcher.GetSingularInfo().extensions > 0, test.eligible);
  }
}

TEST(SingularExtensions, MultiCutAndNegativeExtension) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  DepthCondition condition{8};
  for (const Eval beta : {-11000, -10001}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    table->SetEntry(position, move, -10000, 4, Bound::kLower, 0);
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kPV, DepthCondition>{
        searcher, {8, 7, beta - 1, beta, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, beta);
    EXPECT_EQ(searcher.GetSingularInfo().multi_cuts, beta == -11000 ? 1u : 0u);
    EXPECT_EQ(searcher.GetSingularInfo().negative_extensions,
              beta == -10001 ? 1u : 0u);
    EXPECT_EQ(searcher.GetSingularInfo().extensions, 0u);
  }
}

TEST(SingularExtensions, CancelledSearchPreservesEntryAndPosition) {
  auto stop = std::make_shared<std::atomic_bool>(true);
  DepthCondition condition{8, stop};
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  table->SetEntry(position, move, 10000, 4, Bound::kLower, 0);
  Searcher searcher(position, table);
  const auto result = SearchNode<NodeType::kPV, DepthCondition>{
      searcher, {8, 7, -20000, 20000, 1}, condition}();
  EXPECT_FALSE(result);
  EXPECT_EQ(searcher.GetSingularInfo().searches, 0u);
  EXPECT_EQ(table->GetNode(position).score, 10000);
  EXPECT_EQ(table->GetNode(position).depth, 4);
  EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
}

TEST(SingularExtensions, FindsMateInTwoAndThree) {
  for (const auto& [fen, distance] : {
           std::pair{"7k/8/5K2/8/8/8/8/1Q6 w - - 0 1", 3},
           std::pair{"7k/8/8/4K3/8/8/8/1Q6 w - - 0 1", 5}}) {
    Searcher searcher(PositionFactory{}(fen));
    searcher.InitStartOfSearch();
    DepthCondition condition{8};
    SearchResult score;
    for (Depth depth = 1; depth <= 8; ++depth) {
      score = searcher.Search<NodeType::kPV>(condition, depth, depth,
                                           kMateValue, -kMateValue);
      ASSERT_TRUE(score);
    }
    EXPECT_EQ(*score, -kMateValue - distance) << fen;
  }
}

TEST(SingularExtensions, ReportsTriggerCounts) {
  const auto* net = std::getenv("SCE_SINGULAR_STATS_NET");
  if (!net) {
    GTEST_SKIP() << "Set SCE_SINGULAR_STATS_NET for NNUE trigger measurements";
  }
  ASSERT_TRUE(Nnue::Load(net));
  struct NetworkGuard {
    ~NetworkGuard() { Nnue::Unload(); }
  } guard;
  for (const auto* fen : {
           "r1bqk2r/pp2np1p/2n1p1pb/3pP3/3P1P2/5N2/PP2B1PP/RNBQK2R w KQkq - 0 1",
           "r1bqk2r/pp1pppbp/2n2np1/2p5/2P5/3PPN2/PP3PPP/RNBQKB1R b KQkq - 0 1",
           "r2qkbnr/ppp3p1/2np3p/4p2b/2B1P3/1P3N1P/PBP2PP1/RN1QK2R w KQkq - 0 1"}) {
    Searcher searcher(PositionFactory{}(fen));
    searcher.InitStartOfSearch();
    DepthCondition condition{11};
    std::uint64_t nodes = 0, extensions = 0, multi_cuts = 0;
    for (Depth depth = 1; depth <= 11; ++depth) {
      const auto score = searcher.Search<NodeType::kPV>(
          condition, depth, depth, kMateValue, -kMateValue);
      ASSERT_TRUE(score);
      nodes += searcher.GetInfo().searched_nodes +
               searcher.GetInfo().quiescence_nodes;
      extensions += searcher.GetSingularInfo().extensions;
      multi_cuts += searcher.GetSingularInfo().multi_cuts;
    }
    EXPECT_GT(extensions, 0u);
    std::cout << "singular stats: " << fen << " nodes=" << nodes
              << " extensions=" << extensions << " multi_cuts=" << multi_cuts
              << " extensions_per_1M=" << 1000000.0 * extensions / nodes
              << " multi_cuts_per_1M=" << 1000000.0 * multi_cuts / nodes << '\n';
  }
}
