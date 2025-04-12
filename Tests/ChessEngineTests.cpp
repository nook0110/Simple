#include <gtest/gtest.h>

#include <cstddef>
#include <fstream>
#include <sstream>
#include <vector>

#include "Chess/Move.h"
#include "Chess/MoveFactory.h"
#include "Chess/SimpleChessEngine.h"

using namespace SimpleChessEngine;

namespace ChessEngineTests {
struct BestMoveTestCase {
  BestMoveTestCase(std::string fen, std::string best_move)
      : fen(std::move(fen)), best_move(std::move(best_move)) {}

  std::string fen;
  std::string best_move;
};

class BestMoveTest : public testing::TestWithParam<BestMoveTestCase> {
 protected:
  [[nodiscard]] Move GetMove() const {
    return MoveFactory{}(GetPosition(), GetParam().best_move);
  }

  [[nodiscard]] Position GetPosition() const {
    return PositionFactory{}(GetParam().fen);
  }
};

TEST_P(BestMoveTest, FindBestMove) {
  auto position = GetPosition();

  std::stringstream ss;
  ChessEngine engine(position, ss);
  auto time_for_move = TimeCondition{std::chrono::milliseconds{3000}};
  engine.ComputeBestMove(time_for_move);
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

  size_t solved = {};
  size_t tests = {};

  while (std::getline(positions, line)) {
    ++tests;
    size_t delimeter = line.rfind('#');

    auto position = PositionFactory{}(line.substr(0, delimeter));
    std::vector<Move> best_moves;
    for (const auto& move_str : SplitByWhitespace(line.substr(delimeter + 1))) {
      best_moves.emplace_back(MoveFactory{}(position, move_str));
    }

    ChessEngine engine(position, ss);
    auto time_for_move = TimeCondition{std::chrono::milliseconds{3000}};
    engine.ComputeBestMove(time_for_move);
    if (std::ranges::find(best_moves, engine.GetCurrentBestMove()) !=
        best_moves.end()) {
      ++solved;
    } else {
      LOG(WARNING) << line.substr(0, delimeter) << " "
                   << line.substr(delimeter + 1);
      LOG(WARNING) << engine.GetCurrentBestMove();
    }
  };
  ASSERT_GE(solved, tests * 0.5);
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
        BestMoveTestCase{R"(8/6B1/p5p1/Pp4kp/1P5r/5P1Q/4q1PK/8 w - -)",
                         R"(h3h4)"},
        BestMoveTestCase{R"(8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - -)",
                         R"(e1f1)"}));
}  // namespace ChessEngineTests
