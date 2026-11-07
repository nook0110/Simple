#include <gtest/gtest.h>

#include <cstddef>
#include <memory>

#include "ExitCondition.h"
#include "MoveFactory.h"
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
