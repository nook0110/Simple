#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <memory>

#include "ExitCondition.h"
#include "CheckedMove.h"
#include "MoveFactory.h"
#include "Nnue/Nnue.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "Searcher.h"
#include "Settings.h"

using namespace SimpleChessEngine;

using QTable = Quiescence<DepthCondition>::Table;

TEST(QuiescenceTT, NonPvCutoffsRespectBoundsAndCountNodes) {
  auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  DepthCondition condition{4};
  for (const auto bound : {Bound::kLower, Bound::kUpper, Bound::kExact}) {
    for (const Eval score : {-1234, 1234}) {
      QTable table;
      table.SetEntry(position, move, score, 0, bound, 1);
      Quiescence search{condition, &table, 1};
      const auto result = search.Search<true>(position, -1, 0, 3, false);
      ASSERT_TRUE(result);
      if ((bound & Bound::kLower && score >= 0) ||
          (bound & Bound::kUpper && score < 0)) {
        EXPECT_EQ(*result, score);
      } else {
        EXPECT_NE(*result, score);
      }
      EXPECT_EQ(search.GetSearchedNodes(), 1u);
      EXPECT_EQ(search.Search<true>(position, -1, 0, 3, false), result);
      EXPECT_EQ(search.GetSearchedNodes(), 1u);
    }
  }
}

TEST(QuiescenceTT, PvNodesDoNotCutOffOnDecisiveEntries) {
  auto position = PositionFactory{}();
  QTable table;
  table.SetEntry(position, MoveFactory{}(position, "e2e4"), -kMateValue - 5, 12,
                 Bound::kExact, 1);
  DepthCondition condition{4};
  Quiescence search{condition, &table, 1};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 3, true),
            position.Evaluate());
  EXPECT_EQ(table.GetNode(position).depth, 12);
}

TEST(QuiescenceTT, MainSearchPassesPvFlagAndCountsQuiescenceCutoffs) {
  const auto position = PositionFactory{}();
  auto table = std::make_shared<QTable>();
  table->SetEntry(position, MoveFactory{}(position, "e2e4"), -kMateValue - 5,
                  12, Bound::kExact, 1);
  DepthCondition condition{4};
  Searcher searcher{position, table};
  EXPECT_EQ(searcher.Search<NodeType::kCut>(condition, 0, 0, -1, 0),
            -kMateValue - 5);
  EXPECT_EQ(searcher.GetInfo().quiescence_nodes, 1u);
  EXPECT_EQ(searcher.GetNodes(), 1u);
  EXPECT_EQ(searcher.GetInfo().searched_nodes, 0u);
  EXPECT_EQ(
      searcher.Search<NodeType::kPV>(condition, 0, 0, kMateValue, -kMateValue),
      position.Evaluate());
}

TEST(QuiescenceTT, StoresMoveLessStandPatAndFailLowBounds) {
  auto position = PositionFactory{}();
  const auto value = position.Evaluate();
  DepthCondition condition{4};
  for (const auto bound : {Bound::kLower, Bound::kUpper}) {
    QTable table;
    Quiescence search{condition, &table, 7};
    const auto beta = value + (bound == Bound::kLower ? -10 : 10);
    EXPECT_EQ(search.Search<true>(position, beta - 1, beta, 3, false), value);
    const auto node = table.GetNode(position);
    ASSERT_TRUE(node.IsOccupied());
    EXPECT_FALSE(node.move.IsValid());
    EXPECT_EQ(node.score, value);
    EXPECT_EQ(node.bound, bound);
    EXPECT_EQ(node.depth, 0);
    EXPECT_EQ(node.age, 7);
    EXPECT_EQ(search.Search<true>(position, beta - 1, beta, 3, false), value);
    EXPECT_EQ(search.GetSearchedNodes(), 1u);
  }
}

TEST(QuiescenceTT, QuietTtMoveIsIgnoredOutsideCheck) {
  auto position = PositionFactory{}();
  QTable table;
  table.SetEntry(position, MoveFactory{}(position, "e2e4"), -20000, 0,
                 Bound::kLower, 1);
  DepthCondition condition{4};
  Quiescence search{condition, &table, 1};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 0),
            position.Evaluate());
  EXPECT_EQ(search.GetSearchedNodes(), 1u);
}

TEST(QuiescenceTT, CaptureTtMoveIsSearchedFirstAndStored) {
  auto position = PositionFactory{}("k7/8/8/3p4/3R3p/8/8/K7 w - - 0 1");
  const auto moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kQuiescence>(position);
  ASSERT_GT(moves.size(), 1u);
  const auto tt_move = moves.back();
  ASSERT_TRUE(position.StaticExchangeEvaluation(tt_move, 1));
  const Eval beta = position.Evaluate() + 1;
  QTable table;
  table.SetEntry(position, tt_move, -20000, 0, Bound::kLower, 1);
  auto child = position;
  child.DoMove(tt_move);
  table.SetEntry(child, Move::None(), -beta - 20, 0, Bound::kUpper, 1);
  DepthCondition condition{4};
  Quiescence search{condition, &table, 1};
  EXPECT_EQ(search.Search<true>(position, beta - 1, beta, 0, false), beta + 20);
  EXPECT_EQ(search.GetSearchedNodes(), 2u);
  const auto node = table.GetNode(position);
  EXPECT_EQ(node.move, tt_move);
  EXPECT_EQ(node.score, beta + 20);
  EXPECT_EQ(node.bound, Bound::kLower);
}

TEST(QuiescenceTT, QuietEvasionTtMoveIsSearchedFirstAndStored) {
  auto position = PositionFactory{}("k3r3/8/8/8/8/8/P7/4K3 w - - 0 1");
  ASSERT_TRUE(position.IsUnderCheck());
  const auto moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
  ASSERT_GT(moves.size(), 1u);
  const auto tt_move = moves.back();
  ASSERT_TRUE(tt_move.IsQuiet(position));
  QTable table;
  table.SetEntry(position, tt_move, -20000, 0, Bound::kLower, 1);
  auto child = position;
  child.DoMove(tt_move);
  table.SetEntry(child, Move::None(), -20, 0, Bound::kUpper, 1);
  DepthCondition condition{4};
  Quiescence search{condition, &table, 1};
  EXPECT_EQ(search.Search<true>(position, -1, 0, 0, false), 20);
  EXPECT_EQ(search.GetSearchedNodes(), 2u);
  const auto node = table.GetNode(position);
  EXPECT_EQ(node.move, tt_move);
  EXPECT_EQ(node.score, 20);
  EXPECT_EQ(node.bound, Bound::kLower);
  EXPECT_EQ(node.static_eval, kNoStaticEval);
}

TEST(QuiescenceTT, InCheckEntriesCutOffWithPlyAdjustedMateScores) {
  auto position = PositionFactory{}("k3r3/8/8/8/8/8/P7/4K3 w - - 0 1");
  DepthCondition condition{4};
  for (const Eval value : {kMateValue + 10, -kMateValue - 10,
                           kTablebaseWinValue - 10, -kTablebaseWinValue + 10}) {
    QTable table;
    table.SetEntry(position, Move::None(), value, 0, Bound::kExact, 1);
    Quiescence search{condition, &table, 1};
    EXPECT_EQ(search.Search<true>(position, -1, 0, 4, false),
              value - ((value > 0) - (value < 0)) * 4);
    EXPECT_EQ(search.GetSearchedNodes(), 1u);
  }
}

TEST(QuiescenceTT, StoresMateIndependentOfRootPly) {
  auto position = PositionFactory{}("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
  QTable table;
  DepthCondition condition{4};
  Quiescence search{condition, &table, 1};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 5),
            kMateValue + 5);
  const auto node = table.GetNode(position);
  ASSERT_TRUE(node.IsOccupied());
  EXPECT_EQ(node.score, kMateValue);
  EXPECT_EQ(node.depth, 0);
  EXPECT_EQ(node.bound, Bound::kUpper);
  EXPECT_EQ(search.Search<true>(position, -1, 0, 9, false), kMateValue + 9);
  EXPECT_EQ(search.GetSearchedNodes(), 1u);
}

TEST(QuiescenceTT, DrawAndStopChecksPrecedeTtCutoffs) {
  auto position = PositionFactory{}("7k/8/6K1/8/8/8/8/1Q6 w - - 100 51");
  QTable table;
  table.SetEntry(position, Move::None(), 1234, 0, Bound::kExact, 1);
  auto stop = std::make_shared<std::atomic_bool>(false);
  DepthCondition condition{4, stop};
  Quiescence search{condition, &table, 1};
  EXPECT_EQ(search.Search<true>(position, -1, 0, 3, false), kDrawValue);
  stop->store(true);
  EXPECT_FALSE(search.Search<true>(position, -1, 0, 3, false));
  EXPECT_EQ(table.GetNode(position).score, 1234);
}

class QuiescenceTTNnue : public ::testing::Test {
 protected:
  void SetUp() override {
    const auto* big = std::getenv("SCE_NNUE_BIG");
    const auto* small = std::getenv("SCE_NNUE_SMALL");
    ASSERT_NE(big, nullptr);
    ASSERT_NE(small, nullptr);
    ASSERT_TRUE(Nnue::Load(big));
    ASSERT_TRUE(Nnue::LoadSmall(small));
  }

  void TearDown() override { Nnue::Unload(); }

  static std::uint64_t Context() {
    return (static_cast<std::uint64_t>(Nnue::Generation()) << 32) |
           static_cast<std::uint32_t>(
               Settings::EvaluationParameters::nnue_scale);
  }
};

TEST_F(QuiescenceTTNnue, ReusesStaticEvalWithMatchingContextOnly) {
  auto position = PositionFactory{}();
  const Eval value = position.Evaluate();
  const Eval cached = value + 100;
  DepthCondition condition{4};
  for (const bool matching : {false, true}) {
    QTable table;
    table.SetEntry(position, Move::None(), -20000, 0, Bound::kLower, 1, cached,
                   Context() + !matching);
    Quiescence search{condition, &table, 1};
    EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 0),
              matching ? cached : value);
  }
}

TEST_F(QuiescenceTTNnue, StoresUnrefinedStaticEvalForReuse) {
  auto position = PositionFactory{}();
  const Eval value = position.Evaluate();
  QTable table;
  DepthCondition condition{4};
  Quiescence search{condition, &table, 1};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 0), value);
  const auto node = table.GetNode(position);
  ASSERT_TRUE(node.IsOccupied());
  EXPECT_EQ(table.GetStaticEval(node, Context()), value);
  EXPECT_EQ(node.bound, Bound::kUpper);
}

TEST_F(QuiescenceTTNnue, RefinesStandPatOnlyInBoundDirection) {
  auto position = PositionFactory{}();
  const Eval value = position.Evaluate();
  DepthCondition condition{4};
  for (const auto bound : {Bound::kLower, Bound::kUpper, Bound::kExact}) {
    for (const Eval difference : {-100, 100}) {
      QTable table;
      table.SetEntry(position, Move::None(), value + difference, 0, bound, 1,
                     value, Context());
      Quiescence search{condition, &table, 1};
      const bool refine =
          bound == Bound::kExact ||
          (difference > 0 ? bound == Bound::kLower : bound == Bound::kUpper);
      EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 0),
                refine ? value + difference : value);
      EXPECT_EQ(table.GetStaticEval(table.GetNode(position), Context()), value);
    }
  }
}

TEST(QuiescenceTT, StoresPlyAdjustedMateAndTablebaseCaptureScores) {
  DepthCondition condition{4};
  for (const Eval child_value : {kMateValue, -kTablebaseWinValue}) {
    auto position = PositionFactory{}("k7/8/8/3p4/3R3p/8/8/K7 w - - 0 1");
    const auto tt_move = CheckedMove(position, "d4h4");
    const Eval beta = position.Evaluate() + 1;
    QTable table;
    table.SetEntry(position, tt_move, -20000, 0, Bound::kLower, 1);
    auto child = position;
    child.DoMove(tt_move);
    table.SetEntry(child, Move::None(), child_value, 0, Bound::kUpper, 1);
    Quiescence search{condition, &table, 1};
    EXPECT_EQ(search.Search<true>(position, beta - 1, beta, 4, false),
              -child_value - 5);
    const auto node = table.GetNode(position);
    EXPECT_EQ(node.move, tt_move);
    EXPECT_EQ(node.score, -child_value - 1);
    EXPECT_EQ(node.bound, Bound::kLower);
    EXPECT_EQ(search.Search<true>(position, -1, 0, 9, false),
              -child_value - 10);
    EXPECT_EQ(search.GetSearchedNodes(), 1u);
  }
}

TEST(QuiescenceTT, MoveLessEntryDoesNotSuppressInternalIterativeReduction) {
  const auto position = PositionFactory{}();
  auto table = std::make_shared<QTable>();
  table->SetEntry(position, Move::None(), 0, 0, Bound::kUpper, 0);
  DepthCondition condition{2};
  Searcher searcher{position, table};
  ASSERT_TRUE(
      searcher.Search<NodeType::kPV>(condition, 2, 2, kMateValue, -kMateValue));
  EXPECT_EQ(table->GetNode(position).depth, 1);
}

TEST(QuiescenceTT, CastlingTtMoveIsIgnoredInChess960) {
  DepthCondition condition{4};
  for (const auto* fen :
       {"4k3/8/8/8/8/8/8/6KR w K - 0 1", "4k3/8/8/8/8/8/8/4K1R1 w K - 0 1"}) {
    SCOPED_TRACE(fen);
    auto position = PositionFactory{}(fen);
    const auto move = Move::Make<MoveType::kCastling>(
        position.GetKingSquare(position.GetSideToMove()), 6);
    ASSERT_TRUE(position.IsPseudoLegal(move));
    ASSERT_TRUE(position.IsLegal(move));
    ASSERT_NE(position.GetPieceAt(move.To()), Piece::kNone);
    QTable table;
    table.SetEntry(position, move, -20000, 0, Bound::kLower, 1);
    Quiescence search{condition, &table, 1};
    EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 0),
              position.Evaluate());
    EXPECT_EQ(search.GetSearchedNodes(), 1u);
  }
}
