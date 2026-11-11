#include <gtest/gtest.h>

#include <cstddef>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>
#include <ostream>

#include "ExitCondition.h"
#include "Move.h"
#include "MoveFactory.h"
#include "MoveGenerator.h"
#include "SimpleChessEngine.h"

using namespace SimpleChessEngine;

namespace ChessEngineTests {
struct BestMoveTestCase {
  BestMoveTestCase(std::string fen, std::string best_move, Depth depth = 0)
      : fen(std::move(fen)), best_move(std::move(best_move)), depth(depth) {}

  std::string fen;
  std::string best_move;
  Depth depth;
};

std::ostream& operator<<(std::ostream& os, const BestMoveTestCase& test_case) {
    return os << "FEN: " << test_case.fen << ", Expected move: " << test_case.best_move;
}

class BestMoveTest : public testing::TestWithParam<BestMoveTestCase> {
 protected:
  [[nodiscard]] Move GetMove() const {
    return MoveFactory{}(GetPosition(), GetParam().best_move);
  }

  [[nodiscard]] Position GetPosition() const {
    return PositionFactory{}(GetParam().fen);
  }
};

TEST(RootRepetition, SearchStillReturnsLegalMove) {
  auto position = PositionFactory{}();
  for (const auto *move :
       {"g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1", "f6g8"}) {
    position.DoMove(MoveFactory{}(position, move));
  }
  std::stringstream ss;
  ChessEngine engine(position, ss);
  DepthCondition condition{2};
  engine.ComputeBestMove(condition);
  const auto legal_moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
  EXPECT_NE(std::ranges::find(legal_moves, engine.GetCurrentBestMove()),
            legal_moves.end());
}

TEST(FiftyMoveRule, SearchScoresExpiredClockAsDraw) {
  auto position = PositionFactory{}("7k/8/8/8/8/8/8/KQ6 w - - 100 51");
  Searcher searcher(position);
  searcher.InitStartOfSearch();
  DepthCondition condition{2};
  const auto result = searcher.Search<NodeType::kPV>(
      condition, 2, 2, std::numeric_limits<Eval>::min() / 2,
      std::numeric_limits<Eval>::max() / 2);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, kDrawValue);
}

TEST(FiftyMoveRule, MateOnTheLastMoveStillCounts) {
  auto position = PositionFactory{}("7k/8/6K1/8/8/8/8/1Q6 w - - 99 51");
  Searcher searcher(position);
  searcher.InitStartOfSearch();
  DepthCondition condition{2};
  const auto result = searcher.Search<NodeType::kPV>(
      condition, 2, 2, std::numeric_limits<Eval>::min() / 2,
      std::numeric_limits<Eval>::max() / 2);
  ASSERT_TRUE(result.has_value());
  EXPECT_GT(*result, kTablebaseWinValue);
}

TEST(TimeManagement, ReturnsLegalMoveWithoutTime) {
  auto position = PositionFactory{}(
      "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
  std::stringstream ss;
  ChessEngine engine(position, ss);
  TimeCondition condition{std::chrono::milliseconds{0}};
  engine.ComputeBestMove(condition);
  const auto legal =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
  EXPECT_NE(std::ranges::find(legal, engine.GetCurrentBestMove()), legal.end());
  EXPECT_EQ(ss.str().find("info depth "), std::string::npos);
}

TEST(TimeManagement, StopPreventsDepthIteration) {
  auto stop = std::make_shared<std::atomic_bool>(true);
  DepthCondition condition{10, stop};
  EXPECT_FALSE(condition.ShouldContinueIteration());
}

TEST(TimeManagement, SearchHonorsStopAtFirstNode) {
  Searcher searcher(PositionFactory{}());
  auto stop = std::make_shared<std::atomic_bool>(true);
  DepthCondition condition{1, stop};
  EXPECT_FALSE(searcher.Search<NodeType::kPV>(condition, 1, 1, -10000, 10000));
}

TEST(TimeManagement, QuiescenceHonorsStopAtFirstNode) {
  auto position = PositionFactory{}();
  auto stop = std::make_shared<std::atomic_bool>(true);
  DepthCondition condition{1, stop};
  Quiescence quiescence{condition};
  EXPECT_FALSE(quiescence.Search<true>(position, -10000, 10000, 0));
}

TEST(Pondering, WaitsForPonderhitBeforeBestMove) {
  auto position = PositionFactory{}("7k/8/8/p1p1p1p1/P1P1P1P1/8/8/6K1 b - - 0 1");
  std::stringstream ss;
  ChessEngine engine(position, ss);
  auto stop = std::make_shared<std::atomic_bool>(false);
  Pondering pondering{stop};
  std::thread search([&] { engine.ComputeBestMove(pondering); });
  std::this_thread::sleep_for(std::chrono::milliseconds{300});
  EXPECT_EQ(ss.str().find("bestmove"), std::string::npos);
  pondering.SetCondition(TimeCondition{std::chrono::milliseconds{0}});
  search.join();
  EXPECT_NE(ss.str().find("bestmove"), std::string::npos);
}

TEST(BestMove, PrintsNullMoveWithoutLegalMoves) {
  auto position = PositionFactory{}("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
  std::stringstream ss;
  ChessEngine engine(position, ss);
  DepthCondition condition{3};
  engine.ComputeBestMove(condition);
  EXPECT_NE(ss.str().find("bestmove 0000"), std::string::npos);
}

TEST_P(BestMoveTest, FindBestMove) {
  auto position = GetPosition();

  std::stringstream ss;
  ChessEngine engine(position, ss);
  if (GetParam().depth) {
    DepthCondition condition{GetParam().depth};
    engine.ComputeBestMove(condition);
  } else {
    auto time_for_move = TimeCondition{std::chrono::milliseconds{3000}};
    engine.ComputeBestMove(time_for_move);
  }
  ASSERT_EQ(engine.GetCurrentBestMove(), GetMove());
}

static std::vector<std::string> SplitByWhitespace(const std::string& str) {
  std::istringstream iss(str);
  std::vector<std::string> tokens;
  std::string token;

  while (iss >> token) {
    tokens.push_back(token);
  }

  return tokens;
}

TEST(BestMove, WinAtChess) {
  std::ifstream positions("WinAtChess.test");
  std::string line;
  std::stringstream ss;

  while (std::getline(positions, line)) {
    size_t delimeter = line.rfind('#');

    auto position = PositionFactory{}(line.substr(0, delimeter));
    std::vector<Move> best_moves;
    for (const auto& move_str : SplitByWhitespace(line.substr(delimeter + 1))) {
      best_moves.emplace_back(MoveFactory{}(position, move_str));
    }

    ChessEngine engine(position, ss);
    auto time_for_move = TimeCondition{std::chrono::milliseconds{3000}};
    engine.ComputeBestMove(time_for_move);
    ASSERT_TRUE(std::ranges::find(best_moves, engine.GetCurrentBestMove()) !=
                best_moves.end());
  };
}

INSTANTIATE_TEST_CASE_P(
    EPD, BestMoveTest,
    ::testing::Values(
        BestMoveTestCase{R"(1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -)",
                         R"(d6d1)"},
        BestMoveTestCase{R"(8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -)", R"(a1b1)"}));
INSTANTIATE_TEST_CASE_P(
    NMP, BestMoveTest,
    ::testing::Values(
        BestMoveTestCase{R"(1q1k4/2Rr4/8/2Q3K1/8/8/8/8 w - -)", R"(g5h6)"},
        BestMoveTestCase{R"(8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - -)",
                         R"(e1f1)", 16}));
INSTANTIATE_TEST_CASE_P(DISABLED_NMP, BestMoveTest,
                        ::testing::Values(BestMoveTestCase{
                            R"(8/6B1/p5p1/Pp4kp/1P5r/5P1Q/4q1PK/8 w - -)",
                            R"(h3h4)"}));
}  // namespace ChessEngineTests
