/*
#include <gtest/gtest.h>

#include "Chess/MoveFactory.h"
#include "Chess/MoveGenerator.h"
#include "Chess/Position.h"
#include "Chess/PositionFactory.h"

using namespace SimpleChessEngine;

namespace PositionTests {
TEST(DoMove, DoAndUndoEqualZero) {
  const auto start_pos = PositionFactory{}();
  Position pos = start_pos;

  for (const auto moves =
           MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kDefault>(pos);
       const auto &move : moves) {
    const auto irreversible_data = pos.GetIrreversibleData();
    pos.DoMove(move);
    pos.UndoMove(move, irreversible_data);

    ASSERT_EQ(pos, start_pos);
  }
}

TEST(SomeMoves, DifferentHash) {
  auto pos = PositionFactory{}();

  std::vector<std::string> moves = {"b1c3", "d7d5", "a1b1"};
  std::set<Hash> hashes = {pos.GetHash()};
  for (const auto &move_str : moves) {
    auto move = MoveFactory{}(pos, move_str);
    pos.DoMove(move);
    ASSERT_TRUE(hashes.insert(pos.GetHash()).second);
  }
}

Position DoMoves(Position pos, const std::vector<std::string> &moves) {
  for (const auto &move_str : moves) {
    auto move = MoveFactory{}(pos, move_str);

    pos.DoMove(move);
  }
  return pos;
}

TEST(TwoSimiliarPositions, DifferentHash) {
  const auto start_pos = PositionFactory{}();

  std::vector<std::string> first_moves_str = {"e2e4", "e7e5", "d2d4", "c7c5",
                                              "d4e5"};
  std::vector<std::string> second_moves_str = {"e2e4", "e7e5", "d2d4", "c7c5",
                                               "d4c5"};

  const auto first_position = DoMoves(start_pos, first_moves_str);
  const auto second_position = DoMoves(start_pos, second_moves_str);

  ASSERT_NE(first_position.GetHash(), second_position.GetHash());
}
} // namespace PositionTests
*/