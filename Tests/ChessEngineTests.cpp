#include <gtest/gtest.h>

#include <sstream>

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

  auto time_for_move = TimeCondition{std::chrono::milliseconds{3000}};

  std::stringstream ss;
  ChessEngine engine(position, ss);
  engine.ComputeBestMove(time_for_move);
  ASSERT_EQ(engine.GetCurrentBestMove(), GetMove());
}

INSTANTIATE_TEST_CASE_P(
    EPD, BestMoveTest,
    ::testing::Values(
        BestMoveTestCase{R"(1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -)",
                         R"(d6d1)"},
        BestMoveTestCase{R"(8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -)", R"(a1b1)"}));
}  // namespace ChessEngineTests
