#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "MoveFactory.h"
#include "MoveGenerator.h"
#include "Nnue/Nnue.h"
#include "Position.h"
#include "PositionFactory.h"

namespace SimpleChessEngine {
struct NnueTestAccess {
  static bool Verify(const Position& position) {
    return Nnue::VerifyAccumulators(position, position.nnue_);
  }
};
}  // namespace SimpleChessEngine
using namespace SimpleChessEngine;

namespace NnueTests {
class NnueIncremental : public ::testing::TestWithParam<const char*> {
 protected:
  void SetUp() override {
    const char* path = std::getenv("SCE_NNUE");
    if (path == nullptr) GTEST_SKIP() << "SCE_NNUE is not set";
    ASSERT_TRUE(Nnue::Load(path));
  }

  void TearDown() override { Nnue::Unload(); }
};

[[nodiscard]] bool MatchesFullRefresh(const Position& position) {
  const auto incremental = position.EvaluateNetwork();
  const auto scratch = Nnue::EvaluateFromScratch(position);
  return NnueTestAccess::Verify(position) && incremental.psqt == scratch.psqt &&
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
  RecordProperty("positions_checked", static_cast<int>(checked));
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

TEST_P(NnueIncremental, SpecialMovesMatchFullRefresh) {
  const std::pair<const char*, const char*> cases[] = {
      {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1g1"},
      {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1c1"},
      {"r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", "e8g8"},
      {"r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", "e8c8"},
      {"rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3", "e5f6"},
      {"7k/8/8/8/3pP3/8/8/K7 b - e3 37 42", "d4e3"},
      {"1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7b8q"},
      {"1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7b8r"},
      {"1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7b8b"},
      {"1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7b8n"},
      {"4k3/P7/8/8/8/8/7p/4K3 w - - 0 1", "a7a8q"},
      {"4k3/P7/8/8/8/8/7p/4K3 w - - 0 1", "a7a8n"},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1", "e1f1"},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1", "e1d1"},
  };
  for (const auto& [fen, coordinate] : cases) {
    Position position = PositionFactory{}(fen);
    ASSERT_TRUE(MatchesFullRefresh(position)) << fen;
    const auto move = MoveFactory{}(position, coordinate);
    ASSERT_TRUE(position.IsLegal(move)) << fen << " " << coordinate;
    const auto data = position.GetIrreversibleData();
    position.DoMove(move);
    ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " " << coordinate;
    position.UndoMove(move, data);
    ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " undo " << coordinate;
  }
}

TEST_P(NnueIncremental, MatchesStockfish19Oracle) {
  std::ifstream input("Stockfish19Nnue.test");
  ASSERT_TRUE(input.good());
  int checked = 0;
  for (std::string line; std::getline(input, line);) {
    const auto separator = line.find(" # ");
    ASSERT_NE(separator, std::string::npos);
    const auto fen = line.substr(0, separator);
    std::istringstream expected(line.substr(separator + 3));
    int psqt, positional;
    std::string processed;
    ASSERT_TRUE(bool(expected >> psqt >> positional >> processed));
    const auto position = PositionFactory{}(fen);
    const auto output = position.EvaluateNetwork();
    EXPECT_EQ(output.psqt, psqt) << fen;
    EXPECT_EQ(output.positional, positional) << fen;
    if (processed != "check") {
      EXPECT_EQ(Nnue::PostProcess(position, output), std::stoi(processed))
          << fen;
    }
    ASSERT_TRUE(MatchesFullRefresh(position)) << fen;
    ++checked;
  }
  EXPECT_EQ(checked, 148);
}

TEST_P(NnueIncremental, LazyUpdatesReloadAndFailedLoad) {
  Position position = PositionFactory{}();
  std::mt19937_64 random(20261111);
  std::vector<std::pair<Move, Position::IrreversibleData>> history;
  for (int ply = 0; ply < 120; ++ply) {
    if (ply % 11 == 0) {
      ASSERT_TRUE(MatchesFullRefresh(position));
    }
    const auto moves =
        MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
    if (moves.empty()) break;
    const auto move = moves[random() % moves.size()];
    history.emplace_back(move, position.GetIrreversibleData());
    position.DoMove(move);
  }
  ASSERT_TRUE(MatchesFullRefresh(position));
  const auto generation = Nnue::Generation();
  const auto before = position.EvaluateNetwork();
  EXPECT_FALSE(Nnue::Load("/no-such-nnue19-network"));
  EXPECT_EQ(generation, Nnue::Generation());
  EXPECT_EQ(before.positional, position.EvaluateNetwork().positional);
  ASSERT_TRUE(Nnue::Load(std::getenv("SCE_NNUE")));
  ASSERT_TRUE(MatchesFullRefresh(position));
  while (!history.empty()) {
    position.UndoMove(history.back().first, history.back().second);
    history.pop_back();
    ASSERT_TRUE(MatchesFullRefresh(position));
  }
  Nnue::Unload();
  for (int ply = 0; ply < 8; ++ply) {
    const auto moves =
        MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
    const auto move = moves[random() % moves.size()];
    history.emplace_back(move, position.GetIrreversibleData());
    position.DoMove(move);
  }
  ASSERT_TRUE(Nnue::Load(std::getenv("SCE_NNUE")));
  ASSERT_TRUE(MatchesFullRefresh(position));
  while (!history.empty()) {
    position.UndoMove(history.back().first, history.back().second);
    history.pop_back();
    ASSERT_TRUE(MatchesFullRefresh(position));
  }
}

TEST(NnueScale, UsesStockfish19MaterialAnchor) {
  constexpr double pawn =
      -142.72052667 + 372.35176398 - 340.71073572 + 415.23490212;
  EXPECT_EQ(Nnue::DefaultScale(), static_cast<int>(82.0 * 1024 / pawn + 0.5));
  EXPECT_NEAR(pawn * Nnue::DefaultScale() / 1024, 82.0, 0.03);
}

INSTANTIATE_TEST_SUITE_P(Networks, NnueIncremental,
                         ::testing::Values("SCE_NNUE"));
}  // namespace NnueTests
