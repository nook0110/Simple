#include <gtest/gtest.h>

#include <cstdlib>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "MoveGenerator.h"
#include "Nnue/Nnue.h"
#include "Position.h"
#include "PositionFactory.h"

using namespace SimpleChessEngine;

namespace NnueTests {
class NnueIncremental : public ::testing::TestWithParam<const char*> {
 protected:
  void SetUp() override {
    const char* path = std::getenv(GetParam());
    if (path == nullptr || !Nnue::Load(path)) {
      GTEST_SKIP() << GetParam() << " does not point to a loadable network";
    }
  }

  void TearDown() override { Nnue::Unload(); }
};

[[nodiscard]] bool MatchesFullRefresh(const Position& position) {
  const auto incremental = position.EvaluateNetwork();
  const auto scratch = Nnue::EvaluateFromScratch(position);
  return incremental.psqt == scratch.psqt &&
         incremental.positional == scratch.positional;
}

TEST_P(NnueIncremental, RandomGamesMatchFullRefresh) {
  const std::vector<std::string> fens = {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
      "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
      "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
      "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
      "1k6/1P6/8/8/8/8/6p1/3K4 w - - 0 1"};
  std::mt19937_64 random(20261107);
  size_t checked = 0;
  for (const auto& fen : fens) {
    for (int game = 0; game < 16; ++game) {
      Position position = PositionFactory{}(fen);
      std::vector<std::pair<Move, Position::IrreversibleData>> history;
      for (int ply = 0; ply < 160; ++ply) {
        ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " ply " << ply;
        ++checked;
        const auto moves =
            MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
        if (moves.empty()) break;
        const auto roll = random() % 12;
        if (roll == 0 && !history.empty()) {
          const auto undo_count = 1 + random() % 3;
          for (size_t i = 0; i < undo_count && !history.empty(); ++i) {
            position.UndoMove(history.back().first, history.back().second);
            history.pop_back();
            ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " undo";
          }
          continue;
        }
        if (roll == 1 && !position.IsUnderCheck()) {
          const auto data = position.GetIrreversibleData();
          position.DoMove(NullMove{});
          ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " null move";
          position.UndoMove(NullMove{}, data);
        }
        if (roll == 2) {
          Position copy = position;
          ASSERT_TRUE(MatchesFullRefresh(copy)) << fen << " copy";
          const auto move = moves[random() % moves.size()];
          const auto data = copy.GetIrreversibleData();
          copy.DoMove(move);
          ASSERT_TRUE(MatchesFullRefresh(copy)) << fen << " copy move";
          copy.UndoMove(move, data);
          ASSERT_TRUE(MatchesFullRefresh(copy)) << fen << " copy undo";
          if (!history.empty()) {
            copy.UndoMove(history.back().first, history.back().second);
            ASSERT_TRUE(MatchesFullRefresh(copy)) << fen << " copy underflow";
          }
          ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " after copy";
        }
        const auto move = moves[random() % moves.size()];
        history.emplace_back(move, position.GetIrreversibleData());
        position.DoMove(move);
      }
      while (!history.empty()) {
        position.UndoMove(history.back().first, history.back().second);
        history.pop_back();
        ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " unwind";
      }
    }
  }
  EXPECT_GT(checked, 5000u);
}

TEST_P(NnueIncremental, EvaluationIsSideRelative) {
  const auto white = PositionFactory{}(
      "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3");
  const auto black = PositionFactory{}(
      "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 2 3");
  const auto white_output = white.EvaluateNetwork();
  const auto black_output = black.EvaluateNetwork();
  EXPECT_EQ(white_output.psqt, black_output.psqt);
  EXPECT_EQ(white_output.positional, black_output.positional);
}

INSTANTIATE_TEST_SUITE_P(Networks, NnueIncremental,
                         ::testing::Values("SCE_NNUE_SMALL", "SCE_NNUE_BIG"));
}  // namespace NnueTests
