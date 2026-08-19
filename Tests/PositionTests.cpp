#include <gtest/gtest.h>

#include <limits>

#include "MoveFactory.h"
#include "MoveGenerator.h"
#include "ExitCondition.h"
#include "NodeType.h"
#include "Position.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "Searcher.h"

using namespace SimpleChessEngine;

namespace PositionTests {
TEST(DoMove, DoAndUndoEqualZero) {
  const auto start_pos = PositionFactory{}();
  Position pos = start_pos;

  for (const auto moves =
           MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(pos);
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

TEST(FenFactory, PreservesEnPassantSquare) {
  const std::string fen = "7k/8/8/8/3pP3/8/8/K7 b - e3 37 42";
  const auto position = PositionFactory{}(fen);

  EXPECT_EQ(position.GetEnCroissantSquare(), GetSquareIndex(4, 2));
  EXPECT_EQ(position.GetHalfMoveClock(), 37);
  EXPECT_EQ(position.GetFullMoveNumber(), 42);
  EXPECT_EQ(FenFactory{}(position), fen);
}

TEST(FenFactory, UpdatesCountersFromInitialFenState) {
  auto position = PositionFactory{}(
      "7k/8/8/8/8/8/8/K7 b - - 37 42");
  position.DoMove(MoveFactory{}(position, "h8g7"));
  EXPECT_EQ(position.GetHalfMoveClock(), 38);
  EXPECT_EQ(position.GetFullMoveNumber(), 43);
  position.DoMove(MoveFactory{}(position, "a1b1"));
  EXPECT_EQ(position.GetHalfMoveClock(), 39);
  EXPECT_EQ(position.GetFullMoveNumber(), 43);
}

TEST(FiftyMoveClock, QuietPromotionResetsCounter) {
  auto position = PositionFactory{}(
      "7k/P7/8/8/8/8/8/K7 w - - 99 42");
  position.DoMove(MoveFactory{}(position, "a7a8q"));
  EXPECT_EQ(position.GetHalfMoveClock(), 0);
  EXPECT_EQ(position.GetFullMoveNumber(), 42);
  position.DoMove(MoveFactory{}(position, "h8g7"));
  EXPECT_EQ(position.GetHalfMoveClock(), 1);
  EXPECT_EQ(position.GetFullMoveNumber(), 43);
}

TEST(InsufficientMaterial, DetectsDeadPositions) {
  const std::array dead_positions = {
      "7k/8/8/8/8/8/8/K7 w - - 0 1",
      "7k/8/8/8/8/8/8/KB6 w - - 0 1",
      "7k/8/8/8/8/8/8/KN6 w - - 0 1",
      "2b4k/8/8/8/8/8/8/KB6 w - - 0 1",
      "2b4k/8/8/8/8/3B4/8/KB6 w - - 0 1",
  };

  for (const auto *fen : dead_positions) {
    EXPECT_TRUE(PositionFactory{}(fen).IsInsufficientMaterial()) << fen;
  }
}

TEST(InsufficientMaterial, KeepsPositionsWhereMateIsPossible) {
  const std::array live_positions = {
      "7k/8/8/8/8/8/P7/K7 w - - 0 1",
      "7k/8/8/8/8/8/R7/K7 w - - 0 1",
      "7k/8/8/8/8/8/Q7/K7 w - - 0 1",
      "7k/8/8/8/8/8/8/KNN5 w - - 0 1",
      "7k/8/8/8/8/8/8/KBN5 w - - 0 1",
      "1b5k/8/8/8/8/8/8/KB6 w - - 0 1",
  };

  for (const auto *fen : live_positions) {
    EXPECT_FALSE(PositionFactory{}(fen).IsInsufficientMaterial()) << fen;
  }
}

TEST(InsufficientMaterial, SearchReturnsDrawAndLegalRootMove) {
  auto position =
      PositionFactory{}("7k/8/8/8/8/8/8/K7 w - - 0 1");
  Searcher searcher(position);
  searcher.InitStartOfSearch();
  DepthCondition condition{1};

  const auto result = searcher.Search<NodeType::kPV>(
      condition, 1, 1, std::numeric_limits<Eval>::min() / 2,
      std::numeric_limits<Eval>::max() / 2);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, kDrawValue);
  const auto legal_moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
  EXPECT_NE(std::ranges::find(legal_moves, searcher.GetCurrentBestMove()),
            legal_moves.end());
}

TEST(InsufficientMaterial, QuiescenceReturnsDraw) {
  auto position =
      PositionFactory{}("7k/8/8/8/8/8/8/KB6 w - - 0 1");
  DepthCondition condition{1};
  Quiescence quiescence{condition};

  const auto result = quiescence.Search<true>(
      position, std::numeric_limits<Eval>::min() / 2,
      std::numeric_limits<Eval>::max() / 2, 0);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, kDrawValue);
}
}  // namespace PositionTests
