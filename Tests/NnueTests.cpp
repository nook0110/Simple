#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "CheckedMove.h"
#include "MoveGenerator.h"
#include "Nnue/FeatureTransformer.h"
#include "Nnue/Nnue.h"
#include "Position.h"
#include "PositionFactory.h"

using namespace SimpleChessEngine;

namespace NnueTests {
class NnueIncremental : public ::testing::TestWithParam<const char*> {
 protected:
  void SetUp() override {
    const bool dual = std::string(GetParam()) == "SCE_NNUE_DUAL";
    const char* path = std::getenv(dual ? "SCE_NNUE_BIG" : GetParam());
    if (path == nullptr || !Nnue::Load(path)) {
      GTEST_SKIP() << GetParam() << " does not point to a loadable network";
    }
    if (dual) {
      const char* small = std::getenv("SCE_NNUE_SMALL");
      ASSERT_NE(small, nullptr);
      ASSERT_TRUE(Nnue::LoadSmall(small));
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
      std::vector<std::pair<LegalMove, Position::IrreversibleData>> history;
      for (int ply = 0; ply < 160; ++ply) {
        ASSERT_TRUE(MatchesFullRefresh(position)) << fen << " ply " << ply;
        ++checked;
        const auto moves =
            MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(position);
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
                         ::testing::Values("SCE_NNUE_SMALL", "SCE_NNUE_BIG",
                                           "SCE_NNUE_DUAL"));

class NnueDual : public ::testing::Test {
 protected:
  void SetUp() override {
    big_ = std::getenv("SCE_NNUE_BIG");
    small_ = std::getenv("SCE_NNUE_SMALL");
    if (!big_ || !small_) GTEST_SKIP() << "Both NNUE fixtures are required";
    ASSERT_TRUE(Nnue::Load(big_));
    ASSERT_TRUE(Nnue::LoadSmall(small_));
  }

  void TearDown() override { Nnue::Unload(); }

  const char* big_ = nullptr;
  const char* small_ = nullptr;
};

TEST(NnueMaterial, UsesMiddlegameMaterialRelativeToSideToMove) {
  EXPECT_EQ(PositionFactory{}().SimpleMaterialEval(), 0);
  EXPECT_EQ(PositionFactory{}("4k3/8/8/8/8/8/PP6/R3K3 w - - 0 1")
                .SimpleMaterialEval(), 477 + 2 * 82);
  EXPECT_EQ(PositionFactory{}("4k3/8/8/8/8/8/PP6/R3K3 b - - 0 1")
                .SimpleMaterialEval(), -477 - 2 * 82);
}

TEST_F(NnueDual, KeepsUnusedAccumulatorsLazyAndUpdatesAcrossGaps) {
  auto position = PositionFactory{}();
  Nnue::AccumulatorStack stack;
  auto output = Nnue::Evaluate(position, stack);
  auto scratch = Nnue::EvaluateFromScratch(position);
  EXPECT_EQ(output.psqt, scratch.psqt);
  EXPECT_EQ(output.positional, scratch.positional);
  EXPECT_FALSE(stack.Accumulators<Nnue::kSmallDimensions>()[0].computed[0]);
  EXPECT_TRUE(stack.Accumulators<Nnue::kBigDimensions>()[0].computed[0]);
  const std::array<Move, 4> moves = {Move{12, 28}, Move{52, 36},
                                    Move{6, 21}, Move{57, 42}};
  for (const auto& move : moves) {
    auto& dirty = stack.Push();
    dirty.Add(position.GetPieceAt(move.From()), position.GetSideToMove(),
              move.From(), move.To());
    position.DoMove(CheckedMove(position, move));
  }
  output = Nnue::Evaluate(position, stack);
  scratch = Nnue::EvaluateFromScratch(position);
  EXPECT_EQ(output.psqt, scratch.psqt);
  EXPECT_EQ(output.positional, scratch.positional);
  for (std::size_t i = 0; i < stack.Size(); ++i) {
    EXPECT_TRUE(stack.Accumulators<Nnue::kBigDimensions>()[i].computed[0]);
    EXPECT_FALSE(stack.Accumulators<Nnue::kSmallDimensions>()[i].computed[0]);
  }

  auto winning = PositionFactory{}("4k3/8/8/8/8/8/8/R2QK3 w - - 0 1");
  Nnue::AccumulatorStack winning_stack;
  output = Nnue::Evaluate(winning, winning_stack);
  scratch = Nnue::EvaluateFromScratch(winning);
  EXPECT_EQ(output.psqt, scratch.psqt);
  EXPECT_EQ(output.positional, scratch.positional);
  EXPECT_TRUE(winning_stack.Accumulators<Nnue::kSmallDimensions>()[0].computed[0]);
  EXPECT_FALSE(winning_stack.Accumulators<Nnue::kBigDimensions>()[0].computed[0]);
  for (int i = 0; i < 3; ++i) {
    winning_stack.Push();
    winning.DoMove(NullMove{});
  }
  output = Nnue::Evaluate(winning, winning_stack);
  scratch = Nnue::EvaluateFromScratch(winning);
  EXPECT_EQ(output.psqt, scratch.psqt);
  EXPECT_EQ(output.positional, scratch.positional);
  for (std::size_t i = 0; i < winning_stack.Size(); ++i) {
    EXPECT_TRUE(winning_stack.Accumulators<Nnue::kSmallDimensions>()[i].computed[0]);
    EXPECT_FALSE(winning_stack.Accumulators<Nnue::kBigDimensions>()[i].computed[0]);
  }
}

TEST_F(NnueDual, SwitchingNetworksReusesSameNetworkAncestor) {
  auto position = PositionFactory{}("r3k3/8/8/8/8/8/8/R3K3 w - - 0 1");
  Nnue::AccumulatorStack stack;
  static_cast<void>(Nnue::Evaluate(position, stack));
  const std::array<Move, 4> moves = {Move{0, 56}, Move{60, 51},
                                    Move{56, 59}, Move{51, 59}};
  for (std::size_t i = 0; i < moves.size(); ++i) {
    const auto move = moves[i];
    auto& dirty = stack.Push();
    dirty.Add(position.GetPieceAt(move.From()), position.GetSideToMove(),
              move.From(), move.To());
    const auto captured = position.GetPieceAt(move.To());
    if (captured != Piece::kNone) {
      dirty.Add(captured, Flip(position.GetSideToMove()), move.To(),
                Nnue::kNoSquare);
    }
    position.DoMove(CheckedMove(position, move));
    const auto output = Nnue::Evaluate(position, stack);
    const auto scratch = Nnue::EvaluateFromScratch(position);
    EXPECT_EQ(output.psqt, scratch.psqt);
    EXPECT_EQ(output.positional, scratch.positional);
    const bool last = i + 1 == moves.size();
    EXPECT_EQ(stack.Accumulators<Nnue::kBigDimensions>()[i + 1].computed[0],
              last);
    EXPECT_EQ(stack.Accumulators<Nnue::kSmallDimensions>()[i + 1].computed[0],
              !last);
  }
  for (std::size_t i = 0; i < stack.Size(); ++i) {
    EXPECT_TRUE(stack.Accumulators<Nnue::kBigDimensions>()[i].computed[0]);
  }
}

TEST_F(NnueDual, SelectsSmallAndFallsBackOnlyOnMagnitude) {
  std::vector<Position> positions;
  positions.reserve(256);
  std::vector<Nnue::NetworkOutput> small_outputs;
  ASSERT_TRUE(Nnue::Load(small_));
  std::mt19937_64 random(9171);
  auto position = PositionFactory{}("4k3/8/8/8/8/8/8/R2QK3 w - - 0 1");
  for (int i = 0; i < 256; ++i) {
    positions.push_back(position);
    small_outputs.push_back(position.EvaluateNetwork());
    const auto moves =
        MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(position);
    if (moves.empty() || i % 32 == 31) {
      position = PositionFactory{}(
          "3rk3/8/8/8/8/8/8/3QK3 w - - 0 1");
    } else {
      position.DoMove(moves[random() % moves.size()]);
    }
  }
  ASSERT_TRUE(Nnue::Load(big_));
  ASSERT_TRUE(Nnue::LoadSmall(small_));
  int small_count = 0;
  int fallback_count = 0;
  int big_count = 0;
  for (std::size_t i = 0; i < positions.size(); ++i) {
    Nnue::AccumulatorStack stack;
    const auto output = Nnue::Evaluate(positions[i], stack);
    const auto expected = small_outputs[i];
    const bool try_small = std::abs(positions[i].SimpleMaterialEval()) > 379;
    const bool fallback = std::abs(expected.psqt + expected.positional) < 236;
    EXPECT_EQ(stack.Accumulators<Nnue::kSmallDimensions>()[0].computed[0],
              try_small);
    EXPECT_EQ(stack.Accumulators<Nnue::kBigDimensions>()[0].computed[0],
              !try_small || fallback);
    if (try_small && !fallback) {
      ++small_count;
      EXPECT_EQ(output.psqt, expected.psqt);
      EXPECT_EQ(output.positional, expected.positional);
    } else {
      if (try_small) {
        ++fallback_count;
      } else {
        ++big_count;
      }
      const auto scratch = Nnue::EvaluateFromScratch(positions[i]);
      EXPECT_EQ(output.psqt, scratch.psqt);
      EXPECT_EQ(output.positional, scratch.positional);
    }
  }
  EXPECT_GT(small_count, 0);
  EXPECT_GT(fallback_count, 0);
  EXPECT_GT(big_count, 0);
}

TEST_F(NnueDual, DisablingSmallPreservesBigOutputAndFailedLoadKeepsNetworks) {
  const auto position = PositionFactory{}("4k3/8/8/8/8/8/8/R2QK3 w - - 0 1");
  const auto small = position.EvaluateNetwork();
  ASSERT_FALSE(Nnue::LoadSmall(big_));
  EXPECT_EQ(position.EvaluateNetwork().psqt, small.psqt);
  EXPECT_EQ(position.EvaluateNetwork().positional, small.positional);
  Nnue::UnloadSmall();
  const auto big = position.EvaluateNetwork();
  ASSERT_TRUE(Nnue::Load(big_));
  const auto reference = position.EvaluateNetwork();
  EXPECT_EQ(big.psqt, reference.psqt);
  EXPECT_EQ(big.positional, reference.positional);
  EXPECT_NE(big.psqt + big.positional, small.psqt + small.positional);
}

template <Nnue::IndexType kDimensions>
void CheckFeatureCounts() {
  auto transformer = std::make_unique<Nnue::FeatureTransformer<kDimensions>>();
  alignas(Nnue::kCacheLineSize) std::int16_t input[kDimensions];
  alignas(Nnue::kCacheLineSize) std::int16_t output[kDimensions];
  std::array<std::int32_t, Nnue::kPsqtBuckets> psqt_input{};
  std::array<std::int32_t, Nnue::kPsqtBuckets> psqt_output{};
  for (Nnue::IndexType i = 0; i < kDimensions; ++i) {
    input[i] = static_cast<std::int16_t>(i * 997);
    for (Nnue::IndexType feature = 0; feature < 3; ++feature) {
      transformer->weights[feature * kDimensions + i] =
          static_cast<std::int16_t>(i * 317 + feature * 1709);
    }
  }
  for (Nnue::IndexType i = 0; i < Nnue::kPsqtBuckets; ++i) {
    psqt_input[i] = static_cast<int>(i) * 107;
    for (Nnue::IndexType feature = 0; feature < 3; ++feature) {
      transformer->psqt_weights[feature * Nnue::kPsqtBuckets + i] =
          static_cast<int>(i * 137 + feature * 2017);
    }
  }
  for (Nnue::IndexType r = 0; r <= 3; ++r) {
    for (Nnue::IndexType a = 0; a <= 3; ++a) {
      Nnue::Features::ChangedIndexList removed, added;
      for (Nnue::IndexType i = 0; i < r; ++i) removed.push_back(i);
      for (Nnue::IndexType i = 0; i < a; ++i) added.push_back(2 - i);
      transformer->ApplyUpdate(input, output, psqt_input.data(),
                               psqt_output.data(), removed, added);
      for (Nnue::IndexType i = 0; i < kDimensions; ++i) {
        int expected = input[i];
        for (const auto feature : removed) {
          expected -= transformer->weights[feature * kDimensions + i];
        }
        for (const auto feature : added) {
          expected += transformer->weights[feature * kDimensions + i];
        }
        ASSERT_EQ(output[i], static_cast<std::int16_t>(expected))
            << r << "," << a << " dimension " << i;
      }
      for (Nnue::IndexType i = 0; i < Nnue::kPsqtBuckets; ++i) {
        int expected = psqt_input[i];
        for (const auto feature : removed) {
          expected -= transformer->psqt_weights[feature * Nnue::kPsqtBuckets + i];
        }
        for (const auto feature : added) {
          expected += transformer->psqt_weights[feature * Nnue::kPsqtBuckets + i];
        }
        ASSERT_EQ(psqt_output[i], expected);
      }
      alignas(Nnue::kCacheLineSize) std::int16_t aliased[kDimensions];
      std::copy_n(input, kDimensions, aliased);
      auto aliased_psqt = psqt_input;
      transformer->ApplyUpdate(aliased, aliased, aliased_psqt.data(),
                               aliased_psqt.data(), removed, added);
      EXPECT_TRUE(std::equal(aliased, aliased + kDimensions, output));
      EXPECT_EQ(aliased_psqt, psqt_output);
    }
  }
}

TEST(NnueUpdates, SmallAllFeatureCounts) {
  CheckFeatureCounts<Nnue::kSmallDimensions>();
}

TEST(NnueUpdates, BigAllFeatureCounts) {
  CheckFeatureCounts<Nnue::kBigDimensions>();
}
}  // namespace NnueTests
