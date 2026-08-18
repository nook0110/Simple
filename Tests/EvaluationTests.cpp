#include <gtest/gtest.h>

#include <cstdlib>

#include "PositionFactory.h"
#include "Settings.h"

using namespace SimpleChessEngine;

namespace EvaluationTests {
TEST(PawnSpans, FrontAndRearAreColorRelative) {
  const auto d4 = GetSquareIndex(3, 3);
  EXPECT_EQ(kPawnSpans.front[static_cast<size_t>(Player::kWhite)][d4],
            kFileBB[3] & (kRankBB[4] | kRankBB[5] | kRankBB[6] | kRankBB[7]));
  EXPECT_EQ(kPawnSpans.rear[static_cast<size_t>(Player::kWhite)][d4],
            kFileBB[3] & (kRankBB[0] | kRankBB[1] | kRankBB[2]));
  EXPECT_EQ(kPawnSpans.front[static_cast<size_t>(Player::kBlack)][d4],
            kFileBB[3] & (kRankBB[0] | kRankBB[1] | kRankBB[2]));
  EXPECT_EQ(kPawnSpans.rear[static_cast<size_t>(Player::kBlack)][d4],
            kFileBB[3] & (kRankBB[4] | kRankBB[5] | kRankBB[6] | kRankBB[7]));
}

TEST(PawnSpans, EveryPrecompiledSpanMatchesReference) {
  for (const auto side : {Player::kWhite, Player::kBlack}) {
    const auto side_index = static_cast<size_t>(side);
    const int forward = side == Player::kWhite ? 1 : -1;
    for (BitIndex square = 0; square < static_cast<BitIndex>(kBoardArea);
         ++square) {
      const auto [file, rank] = GetCoordinates(square);
      Bitboard expected_front;
      Bitboard expected_rear;
      Bitboard expected_passed;
      Bitboard expected_adjacent_files;

      for (BitIndex target = 0; target < static_cast<BitIndex>(kBoardArea);
           ++target) {
        const auto [target_file, target_rank] = GetCoordinates(target);
        const int rank_delta = (target_rank - rank) * forward;
        const int file_delta = std::abs(target_file - file);
        if (target_file == file && rank_delta > 0) {
          expected_front.Set(target);
        }
        if (target_file == file && rank_delta < 0) {
          expected_rear.Set(target);
        }
        if (file_delta <= 1 && rank_delta > 0) {
          expected_passed.Set(target);
        }
        if (file_delta == 1) {
          expected_adjacent_files.Set(target);
        }
      }

      EXPECT_EQ(kPawnSpans.front[side_index][square], expected_front)
          << "side=" << side_index << " square=" << static_cast<int>(square);
      EXPECT_EQ(kPawnSpans.rear[side_index][square], expected_rear)
          << "side=" << side_index << " square=" << static_cast<int>(square);
      EXPECT_EQ(kPawnSpans.passed[side_index][square], expected_passed)
          << "side=" << side_index << " square=" << static_cast<int>(square);
      EXPECT_EQ(kPawnSpans.adjacent_files[square], expected_adjacent_files)
          << "side=" << side_index << " square=" << static_cast<int>(square);
    }
  }
}

TEST(Pawns, PassedPawnBonus) {
  const auto position =
      PositionFactory{}("7k/8/8/3P4/8/8/8/K7 w - - 0 1");
  const Eval baseline = position.Evaluate();
  const Eval original =
      Settings::EvaluationParameters::passed_pawn[4].eval[1];

  Settings::EvaluationParameters::SetPassedPawn(
      4, GamePhase::kEndGame, original + 100);
  EXPECT_EQ(position.Evaluate(), baseline + 100);
  Settings::EvaluationParameters::SetPassedPawn(
      4, GamePhase::kEndGame, original);
}

TEST(Pawns, BlockedPawnIsNotPassed) {
  const auto position =
      PositionFactory{}("7k/8/3p4/3P4/8/8/8/K7 w - - 0 1");
  const Eval baseline = position.Evaluate();
  const Eval original =
      Settings::EvaluationParameters::passed_pawn[4].eval[1];

  Settings::EvaluationParameters::SetPassedPawn(
      4, GamePhase::kEndGame, original + 100);
  EXPECT_EQ(position.Evaluate(), baseline);
  Settings::EvaluationParameters::SetPassedPawn(
      4, GamePhase::kEndGame, original);
}

TEST(Pawns, DoubledPawnPenaltyCountsExtraPawns) {
  const auto position =
      PositionFactory{}("7k/8/8/8/3P4/3P4/8/K7 w - - 0 1");
  const Eval baseline = position.Evaluate();
  const Eval original =
      Settings::EvaluationParameters::doubled_pawn.eval[1];

  Settings::EvaluationParameters::SetDoubledPawn(
      GamePhase::kEndGame, original - 20);
  EXPECT_EQ(position.Evaluate(), baseline - 20);
  Settings::EvaluationParameters::SetDoubledPawn(
      GamePhase::kEndGame, original);
}

TEST(Pawns, IsolatedPawnPenalty) {
  const auto position =
      PositionFactory{}("7k/8/8/8/3P4/8/8/K7 w - - 0 1");
  const Eval baseline = position.Evaluate();
  const Eval original =
      Settings::EvaluationParameters::isolated_pawn.eval[1];

  Settings::EvaluationParameters::SetIsolatedPawn(
      GamePhase::kEndGame, original - 20);
  EXPECT_EQ(position.Evaluate(), baseline - 20);
  Settings::EvaluationParameters::SetIsolatedPawn(
      GamePhase::kEndGame, original);
}

TEST(Pawns, PsqtSquareAdjustmentAppliesToBlockedPawn) {
  const auto position =
      PositionFactory{}("7k/3p4/3P4/8/8/8/8/K7 w - - 0 1");
  const Eval baseline = position.Evaluate();
  const Eval original = Settings::EvaluationParameters::
      pawn_psqt_adjustment[5][3].eval[1];

  Settings::EvaluationParameters::SetPawnPsqtAdjustment(
      5, 3, GamePhase::kEndGame, original - 67);
  EXPECT_EQ(position.Evaluate(), baseline - 67);
  Settings::EvaluationParameters::SetPawnPsqtAdjustment(
      5, 3, GamePhase::kEndGame, original);
}

TEST(Pawns, EvaluationIsColorSymmetric) {
  const auto white = PositionFactory{}(
      "7k/6p1/2p2P2/3P4/8/8/1P6/K7 w - - 0 1");
  const auto black = PositionFactory{}(
      "k7/1p6/8/8/3p4/2P2p2/6P1/7K b - - 0 1");

  EXPECT_EQ(white.Evaluate(), black.Evaluate());
}

TEST(PawnHash, ChangingParametersInvalidatesCachedScore) {
  const auto position =
      PositionFactory{}("7k/8/8/3P4/8/8/8/K7 w - - 0 1");
  const Eval baseline = position.Evaluate();
  const Eval original =
      Settings::EvaluationParameters::passed_pawn[4].eval[1];

  Settings::EvaluationParameters::SetPassedPawn(
      4, GamePhase::kEndGame, original + 100);
  EXPECT_EQ(position.Evaluate(), baseline + 100);
  Settings::EvaluationParameters::SetPassedPawn(
      4, GamePhase::kEndGame, original + 50);
  EXPECT_EQ(position.Evaluate(), baseline + 50);
  Settings::EvaluationParameters::SetPassedPawn(
      4, GamePhase::kEndGame, original);
}

TEST(Material, RawPawnEndgameValue) {
  const auto position =
      PositionFactory{}("7k/8/8/3P4/8/8/8/K7 w - - 0 1");
  const Eval baseline = position.Evaluate();
  const Eval original = Settings::EvaluationParameters::
      material_value[static_cast<size_t>(Piece::kPawn)].eval[1];

  Settings::EvaluationParameters::SetMaterialValue(
      Piece::kPawn, GamePhase::kEndGame, original + 10);
  EXPECT_EQ(position.Evaluate(), baseline + 10);
  Settings::EvaluationParameters::SetMaterialValue(
      Piece::kPawn, GamePhase::kEndGame, original);
}
}  // namespace EvaluationTests
