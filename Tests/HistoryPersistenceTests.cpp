#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <sstream>
#include <string>

#include "ExitCondition.h"
#include "SimpleChessEngine.h"

using namespace SimpleChessEngine;

namespace {
auto ContinuationEntries(const Searcher& searcher) {
  return searcher.GetContinuationHistory() | std::views::join |
         std::views::join | std::views::join;
}

std::uint64_t TotalNodes(const std::string& output) {
  std::istringstream lines(output);
  std::string line;
  std::uint64_t nodes = 0;
  std::uint64_t quiescence_nodes = 0;
  while (std::getline(lines, line)) {
    std::istringstream fields(line);
    std::string tag;
    std::string name;
    std::uint64_t value = 0;
    if (!(fields >> tag >> name >> value) || tag != "info") continue;
    if (name == "nodes") nodes = value;
    if (name == "quiescence_nodes") quiescence_nodes = value;
  }
  return nodes + quiescence_nodes;
}
}

TEST(HistoryPersistence, SecondGoDecaysButterflyAndPreservesContinuation) {
  std::ostringstream output;
  ChessEngine engine(PositionFactory{}(), output);
  DepthCondition first{5};
  engine.ComputeBestMove(first);
  const auto& searcher = engine.GetSearcher();
  auto expected = searcher.GetHistory();
  const auto continuation = searcher.GetContinuationHistory();
  EXPECT_TRUE(std::ranges::any_of(ContinuationEntries(searcher),
                                  [](auto entry) { return entry != 0; }));

  bool positive = false;
  bool negative = false;
  for (const auto& side : expected) {
    for (const auto& from : side) {
      for (const auto entry : from) {
        positive |= entry > 0;
        negative |= entry < 0;
      }
    }
  }
  EXPECT_TRUE(positive);
  EXPECT_TRUE(negative);
  std::size_t killers = 0;
  for (Depth ply = 0; ply < kMaxSearchPly; ++ply) {
    killers += searcher.GetKillers().AvailableKillerCount(ply);
  }
  EXPECT_GT(killers, 0u);

  for (int go = 0; go < 2; ++go) {
    for (auto& side : expected) {
      for (auto& from : side) {
        for (auto& entry : from) entry = entry * 729 / 1024;
      }
    }
    DepthCondition stopped{0};
    engine.ComputeBestMove(stopped);
    EXPECT_EQ(searcher.GetHistory(), expected);
    EXPECT_EQ(searcher.GetContinuationHistory(), continuation);
    for (Depth ply = 0; ply < kMaxSearchPly; ++ply) {
      EXPECT_EQ(searcher.GetKillers().AvailableKillerCount(ply), 0u);
    }
    auto position = searcher.GetPosition();
    const auto side = position.GetSideToMove();
    for (const auto move :
         MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(
             position)) {
      EXPECT_EQ(searcher.GetQuietScore(move, 2),
                expected[side][move.From()][move.To()]);
    }
  }
}

TEST(HistoryPersistence, NewGameClearsBothTables) {
  std::ostringstream output;
  ChessEngine engine(PositionFactory{}(), output);
  DepthCondition condition{5};
  engine.ComputeBestMove(condition);
  const auto& searcher = engine.GetSearcher();
  EXPECT_TRUE(std::ranges::any_of(ContinuationEntries(searcher),
                                  [](auto entry) { return entry != 0; }));

  engine.NewGame();
  for (const auto& side : searcher.GetHistory()) {
    for (const auto& from : side) {
      EXPECT_TRUE(
          std::ranges::all_of(from, [](auto entry) { return entry == 0; }));
    }
  }
  EXPECT_TRUE(std::ranges::all_of(ContinuationEntries(searcher),
                                  [](auto entry) { return entry == 0; }));
}

TEST(HistoryPersistence, RepeatedPositionDoesNotIncreaseNodes) {
  std::ostringstream output;
  ChessEngine engine(PositionFactory{}(), output);
  DepthCondition first{6};
  engine.ComputeBestMove(first);
  const auto first_nodes = TotalNodes(output.str());
  output.str({});
  output.clear();
  DepthCondition second{6};
  engine.ComputeBestMove(second);
  const auto second_nodes = TotalNodes(output.str());
  EXPECT_GT(first_nodes, 0u);
  EXPECT_GT(second_nodes, 0u);
  EXPECT_LE(second_nodes, first_nodes);
}
