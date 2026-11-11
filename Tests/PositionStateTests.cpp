#include <gtest/gtest.h>

#include "MoveFactory.h"
#include "PositionFactory.h"

namespace SimpleChessEngine {
struct PositionStateTestAccess {
  static bool VerifyPinCache(const Position& position) {
    return position.VerifyPinCache();
  }

  static void ClearBlackPins(Position& position) {
    const auto side = static_cast<std::size_t>(Player::kBlack);
    position.irreversible_data_.blockers[side] = kEmptyBoard;
    position.irreversible_data_.pinners[side] = kEmptyBoard;
  }
};

namespace {
void ExpectPinCacheUnchanged(const Position& position) {
  const auto before = position.GetIrreversibleData();
  EXPECT_TRUE(PositionStateTestAccess::VerifyPinCache(position));
  const auto after = position.GetIrreversibleData();
  EXPECT_EQ(before.pins_valid, after.pins_valid);
  EXPECT_EQ(before.blockers, after.blockers);
  EXPECT_EQ(before.pinners, after.pinners);
}

TEST(PositionState, VerifiesPinCacheAcrossMoveAndNullMoveUndo) {
  auto position = PositionFactory{}("4k3/4n3/8/3p4/8/8/8/3QR1K1 b - - 0 1");
  ExpectPinCacheUnchanged(position);
  position.ComputePins(Player::kWhite);
  position.ComputePins(Player::kBlack);
  ExpectPinCacheUnchanged(position);
  const auto before = position.GetIrreversibleData();
  const auto move = MoveFactory{}(position, "e8f8");
  position.DoMove(move);
  ExpectPinCacheUnchanged(position);
  position.ComputePins(Player::kWhite);
  position.ComputePins(Player::kBlack);
  ExpectPinCacheUnchanged(position);
  position.UndoMove(move, before);
  ExpectPinCacheUnchanged(position);
  position.DoMove(NullMove{});
  ExpectPinCacheUnchanged(position);
  position.UndoMove(NullMove{}, before);
  ExpectPinCacheUnchanged(position);
}

TEST(PositionState, DetectsCorruptValidPinCacheWithoutRepairingIt) {
  auto position = PositionFactory{}("4k3/4n3/8/3p4/8/8/8/3QR1K1 b - - 0 1");
  position.ComputePins(Player::kBlack);
  PositionStateTestAccess::ClearBlackPins(position);
  EXPECT_FALSE(PositionStateTestAccess::VerifyPinCache(position));
  const auto after = position.GetIrreversibleData();
  const auto black = static_cast<std::size_t>(Player::kBlack);
  EXPECT_TRUE(after.pins_valid[black]);
  EXPECT_EQ(after.blockers[black], kEmptyBoard);
  EXPECT_EQ(after.pinners[black], kEmptyBoard);
}
}
}
