#include <gtest/gtest.h>

#include <memory>

#include "ExitCondition.h"
#include "MoveFactory.h"
#include "PositionFactory.h"
#include "Searcher.h"
#include "Settings.h"
#include "StaticEvalHistory.h"

using namespace SimpleChessEngine;

TEST(StaticEvalHistory, MissingAncestorsDoNotSetTrends) {
  StaticEvalHistory history;
  const auto root = history.Update(0, 50);
  EXPECT_FALSE(root.improving);
  EXPECT_FALSE(root.opponent_worsening);

  history.Clear();
  const auto child = history.Update(1, 50);
  EXPECT_FALSE(child.improving);
  EXPECT_FALSE(child.opponent_worsening);
}

TEST(StaticEvalHistory, ComparesSameSideAndOppositeSideEvaluations) {
  StaticEvalHistory history;
  const auto root = history.Update(0, -20);
  const auto child = history.Update(1, 10);
  EXPECT_FALSE(root.improving);
  EXPECT_FALSE(child.opponent_worsening);

  const auto improving = history.Update(2, -19);
  EXPECT_TRUE(improving.improving);
  EXPECT_FALSE(improving.opponent_worsening);

  const auto worsening = history.Update(2, -21);
  EXPECT_FALSE(worsening.improving);
  EXPECT_FALSE(worsening.opponent_worsening);

  const auto both = history.Update(2, -9);
  EXPECT_TRUE(both.improving);
  EXPECT_TRUE(both.opponent_worsening);
}

TEST(StaticEvalHistory, EqualEvaluationsDoNotSetTrends) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(0, 20).improving);
  EXPECT_FALSE(history.Update(1, -20).opponent_worsening);
  const auto equal = history.Update(2, 20);
  EXPECT_FALSE(equal.improving);
  EXPECT_FALSE(equal.opponent_worsening);
}

TEST(StaticEvalHistory, CheckClearsSiblingEvaluationAndFallsBackFourPlies) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(0, 20).improving);
  EXPECT_TRUE(history.Update(2, 100).improving);
  const auto checked = history.Update(2, std::nullopt);
  EXPECT_FALSE(checked.improving);
  EXPECT_FALSE(checked.opponent_worsening);

  const auto fallback = history.Update(4, 21);
  EXPECT_TRUE(fallback.improving);
  EXPECT_FALSE(fallback.opponent_worsening);
  EXPECT_FALSE(history.Update(4, 20).improving);
}

TEST(StaticEvalHistory, MissingFallbackDoesNotSetImproving) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(4, 500).improving);
  EXPECT_FALSE(history.Update(3, std::nullopt).opponent_worsening);
  EXPECT_FALSE(history.Update(4, 500).opponent_worsening);
}

TEST(StaticEvalHistory, ResetDiscardsPreviousSearchEvaluations) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(0, -20).improving);
  EXPECT_TRUE(history.Update(1, 21).opponent_worsening);
  history.Clear();
  const auto after_reset = history.Update(2, 50);
  EXPECT_FALSE(after_reset.improving);
  EXPECT_FALSE(after_reset.opponent_worsening);
}

TEST(StaticEvalHistory, SupportsMaximumPly) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(kMaxSearchPly - 2, 20).improving);
  EXPECT_FALSE(history.Update(kMaxSearchPly - 1, -21).opponent_worsening);
  const auto last = history.Update(kMaxSearchPly, 22);
  EXPECT_TRUE(last.improving);
  EXPECT_TRUE(last.opponent_worsening);
}

TEST(EvaluationTrendPruning, ReverseFutilityScalesBothDeductions) {
  using RFP = Settings::PruneParameters::RFPSettings;
  EXPECT_EQ(RFP::Margin(5, false, false, false), 500);
  EXPECT_EQ(RFP::Margin(5, false, true, false), 300);
  EXPECT_EQ(RFP::Margin(5, false, false, true), 467);
  EXPECT_EQ(RFP::Margin(5, false, true, true), 267);
  EXPECT_EQ(RFP::Margin(5, true, false, false), 385);
  EXPECT_EQ(RFP::Margin(5, true, true, false), 231);
  EXPECT_EQ(RFP::Margin(5, true, false, true), 360);
  EXPECT_EQ(RFP::Margin(5, true, true, true), 206);
  EXPECT_EQ(RFP::Margin(1, false, true, true), -133);
}

TEST(EvaluationTrendPruning, LateMoveCountUsesStockfishIntegerDivision) {
  using LMP = Settings::PruneParameters::LMPSettings;
  EXPECT_EQ(LMP::MoveCount(1, false), 2);
  EXPECT_EQ(LMP::MoveCount(1, true), 4);
  EXPECT_EQ(LMP::MoveCount(2, false), 3);
  EXPECT_EQ(LMP::MoveCount(2, true), 7);
  EXPECT_EQ(LMP::MoveCount(8, false), 33);
  EXPECT_EQ(LMP::MoveCount(8, true), 67);
}

TEST(EvaluationTrendPruning, LateMoveReductionScalesOnlyLogarithmicTerm) {
  using LMR = Settings::PruneParameters::LMRSettings;
  EXPECT_DOUBLE_EQ(LMR::ScaleReduction(4.0, true), 4.0);
  EXPECT_DOUBLE_EQ(LMR::ScaleReduction(4.0, false), 5.4921875);
  EXPECT_DOUBLE_EQ(LMR::ScaleReduction(0.0, false), 0.0);
}

TEST(EvaluationTrendPruning, ReverseFutilityRespectsQuietTtMove) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  DepthCondition condition{3};
  for (const bool quiet_tt_move : {false, true}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    if (quiet_tt_move) {
      table->SetEntry(position, move, 0, 0, Bound::kUpper, 0);
    }
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kAll, DepthCondition>{
        searcher, {3, 3, -1001, -1000, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(searcher.GetInfo().searched_nodes == 1, !quiet_tt_move);
  }
}

TEST(EvaluationTrendPruning, ReverseFutilityDoesNotCutLosingTablebaseBounds) {
  const auto position = PositionFactory{}();
  DepthCondition condition{2};
  const Eval loss_bound =
      -kTablebaseWinValue + static_cast<Eval>(kMaxSearchPly);
  for (const Eval beta : {loss_bound, loss_bound + 1}) {
    Searcher searcher(position);
    const auto result = SearchNode<NodeType::kAll, DepthCondition>{
        searcher, {2, 2, beta - 1, beta, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(searcher.GetInfo().searched_nodes == 1, beta > loss_bound);
  }
}
