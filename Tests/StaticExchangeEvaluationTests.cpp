#include <gtest/gtest.h>

#include "CheckedMove.h"
#include "MoveFactory.h"
#include "PositionFactory.h"

using namespace SimpleChessEngine;

namespace {

void ExpectSeeValue(const Position &position, const std::string &move_text,
                    Eval value) {
  const auto move = MoveFactory{}(position, move_text);
  EXPECT_TRUE(position.StaticExchangeEvaluation(move, value)) << move_text;
  EXPECT_FALSE(position.StaticExchangeEvaluation(move, value + 1)) << move_text;
}

TEST(StaticExchangeEvaluation, IgnoresPinnedBlackKnight) {
  const auto position =
      PositionFactory{}("4k3/4n3/8/3p4/8/8/8/3QR1K1 w - - 0 1");
  ExpectSeeValue(position, "d1d5", 90);
  EXPECT_TRUE(position.StaticExchangeEvaluation(
      MoveFactory{}(position, "d1d5"), 0));
}

TEST(StaticExchangeEvaluation, IgnoresPinnedWhiteKnight) {
  const auto position =
      PositionFactory{}("3qr1k1/8/8/8/3P4/8/4N3/4K3 b - - 0 1");
  ExpectSeeValue(position, "d8d4", 90);
  EXPECT_TRUE(position.StaticExchangeEvaluation(
      MoveFactory{}(position, "d8d4"), 0));
}

TEST(StaticExchangeEvaluation, KeepsUnpinnedRecapturer) {
  const auto position =
      PositionFactory{}("4k3/4n3/8/3p4/8/8/8/3Q1RK1 w - - 0 1");
  ExpectSeeValue(position, "d1d5", -870);
}

TEST(StaticExchangeEvaluation, KeepsRecapturerBehindTwoBlockers) {
  const auto position =
      PositionFactory{}("4k3/4n3/8/3pp3/8/8/8/3QR1K1 w - - 0 1");
  ExpectSeeValue(position, "d1d5", -870);
}

TEST(StaticExchangeEvaluation, IgnoresPinnedBishop) {
  const auto position =
      PositionFactory{}("4k3/4b3/3p4/8/8/8/8/3QR1K1 w - - 0 1");
  ExpectSeeValue(position, "d1d6", 90);
}

TEST(StaticExchangeEvaluation, IgnoresPinnedRecapturerOfMovingSide) {
  const auto position =
      PositionFactory{}("6rk/8/8/8/8/1n6/3p2R1/3Q2K1 w - - 0 1");
  ExpectSeeValue(position, "d1d2", -877);
}

TEST(StaticExchangeEvaluation, ReleasesBlockerWhenPinnerMoves) {
  const auto position =
      PositionFactory{}("4k3/8/8/8/4n3/8/3p4/4Q1K1 w - - 0 1");
  ExpectSeeValue(position, "e1d2", -862);
}

TEST(StaticExchangeEvaluation, ReleasesBlockerWhenPinnerIsCaptured) {
  const auto position =
      PositionFactory{}("3Qr3/2n5/7k/8/8/8/4R3/4K3 w - - 0 1");
  ExpectSeeValue(position, "d8e8", -166);
}

TEST(StaticExchangeEvaluation, ReleasesBlockerAfterPinnerRecaptures) {
  const auto position =
      PositionFactory{}("3Q4/7k/3p4/8/1b6/8/3R4/4K3 w - - 0 1");
  ExpectSeeValue(position, "d8d6", -555);
}

TEST(StaticExchangeEvaluation, TracksMultiplePinnersOnKingRay) {
  const auto position =
      PositionFactory{}("4k3/4n3/8/3p4/8/8/4R3/3QR1K1 w - - 0 1");
  ExpectSeeValue(position, "d1d5", 89);
  const auto pins = position.GetIrreversibleData();
  const auto black = static_cast<size_t>(Player::kBlack);
  EXPECT_EQ(pins.pinners[black],
            SingleSquare(GetSquareIndex(4, 0)) |
                SingleSquare(GetSquareIndex(4, 1)));
}

TEST(StaticExchangeEvaluation, KeepsOriginalBlockersWhileAnyPinnerRemains) {
  const auto position =
      PositionFactory{}("4k3/5b2/8/7B/4n3/8/3p4/4Q1K1 w - - 0 1");
  ExpectSeeValue(position, "e1d2", 90);
}

TEST(StaticExchangeEvaluation, RefreshesPinsAfterMoveAndRestoresOnUndo) {
  auto position =
      PositionFactory{}("4k3/4n3/8/3p4/8/8/8/3QR1K1 b - - 0 1");
  position.ComputePins(Player::kWhite);
  position.ComputePins(Player::kBlack);
  const auto pins = position.GetIrreversibleData();
  const auto move = CheckedMove(position, "e8f8");
  position.DoMove(move);
  ExpectSeeValue(position, "d1d5", -870);
  position.UndoMove(move, pins);
  position.SetSideToMove(Player::kWhite);
  ExpectSeeValue(position, "d1d5", 90);
}

TEST(StaticExchangeEvaluation, PreservesPinsAcrossNullMoveAndUndo) {
  auto position =
      PositionFactory{}("4k3/4n3/8/3p4/8/8/8/3QR1K1 w - - 0 1");
  ExpectSeeValue(position, "d1d5", 90);
  const auto pins = position.GetIrreversibleData();
  position.DoMove(NullMove{});
  position.ComputePins(Player::kWhite);
  position.ComputePins(Player::kBlack);
  position.UndoMove(NullMove{}, pins);
  ExpectSeeValue(position, "d1d5", 90);
}

}
