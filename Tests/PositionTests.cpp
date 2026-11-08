#include <gtest/gtest.h>

#include <limits>

#include "MoveFactory.h"
#include "CheckedMove.h"
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
           MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(pos);
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
    auto move = CheckedMove(pos, move_str);
    pos.DoMove(move);
    ASSERT_TRUE(hashes.insert(pos.GetHash()).second);
  }
}

Position DoMoves(Position pos, const std::vector<std::string> &moves) {
  for (const auto &move_str : moves) {
    auto move = CheckedMove(pos, move_str);

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
  position.DoMove(CheckedMove(position, "h8g7"));
  EXPECT_EQ(position.GetHalfMoveClock(), 38);
  EXPECT_EQ(position.GetFullMoveNumber(), 43);
  position.DoMove(CheckedMove(position, "a1b1"));
  EXPECT_EQ(position.GetHalfMoveClock(), 39);
  EXPECT_EQ(position.GetFullMoveNumber(), 43);
}

TEST(FenFactory, CastlingIgnoresRooksOffTheBackRank) {
  auto position =
      PositionFactory{}("3qk2r/prp1pppp/1p1p4/8/8/8/1R2PPPP/4K2R w Kk - 0 1");
  position.DoMove(CheckedMove(position, "e1g1"));
  position.DoMove(CheckedMove(position, "e8g8"));
  EXPECT_EQ(FenFactory{}(position),
            "3q1rk1/prp1pppp/1p1p4/8/8/8/1R2PPPP/5RK1 w - - 2 2");
}

TEST(FenFactory, CastlingUsesCornerRookWhenAnotherRookShares) {
  auto position = PositionFactory{}("r1r1k2r/8/8/8/8/8/8/R3K2R b Kq - 0 1");
  position.DoMove(CheckedMove(position, "c8c7"));
  position.DoMove(CheckedMove(position, "e1g1"));
  position.DoMove(CheckedMove(position, "e8c8"));
  EXPECT_EQ(FenFactory{}(position), "2kr3r/2r5/8/8/8/8/8/R4RK1 w - - 3 3");
}

TEST(Hash, IncludesCastlingRightsFromFen) {
  const auto with_rights =
      PositionFactory{}("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  const auto without_rights =
      PositionFactory{}("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");
  EXPECT_NE(with_rights.GetHash(), without_rights.GetHash());
}

TEST(Hash, LostCastlingRightsMatchFenWithoutThem) {
  auto position = PositionFactory{}("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
  position.DoMove(CheckedMove(position, "h1g1"));
  position.DoMove(CheckedMove(position, "e8d8"));
  position.DoMove(CheckedMove(position, "g1h1"));
  position.DoMove(CheckedMove(position, "d8e8"));
  const auto expected = PositionFactory{}("4k3/8/8/8/8/8/8/4K2R w - - 4 3");
  EXPECT_EQ(position.GetHash(), expected.GetHash());
}

TEST(Repetition, IgnoresEnPassantSquareNobodyCanCapture) {
  auto position = PositionFactory{}();
  for (const auto *move : {"e2e4", "g8f6", "g1f3", "f6g8", "f3g1", "g8f6",
                           "g1f3", "f6g8", "f3g1"}) {
    position.DoMove(CheckedMove(position, move));
  }
  EXPECT_TRUE(position.DetectRepetition(0));
}

TEST(Hash, KeepsCapturableEnPassantSquare) {
  auto position = PositionFactory{}("4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1");
  position.DoMove(CheckedMove(position, "e2e4"));
  const auto with_square =
      PositionFactory{}("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");
  const auto without_square =
      PositionFactory{}("4k3/8/8/8/3pP3/8/8/4K3 b - - 0 1");
  EXPECT_EQ(position.GetHash(), with_square.GetHash());
  EXPECT_NE(position.GetHash(), without_square.GetHash());
}

TEST(FiftyMoveClock, QuietPromotionResetsCounter) {
  auto position = PositionFactory{}(
      "7k/P7/8/8/8/8/8/K7 w - - 99 42");
  position.DoMove(CheckedMove(position, "a7a8q"));
  EXPECT_EQ(position.GetHalfMoveClock(), 0);
  EXPECT_EQ(position.GetFullMoveNumber(), 42);
  position.DoMove(CheckedMove(position, "h8g7"));
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
      MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(position);
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

TEST(Quiescence, ScoresStalemateAsDraw) {
  auto position = PositionFactory{}("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
  DepthCondition condition{1};
  Quiescence quiescence{condition};

  const auto result = quiescence.Search<true>(
      position, std::numeric_limits<Eval>::min() / 2,
      std::numeric_limits<Eval>::max() / 2, 0);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, kDrawValue);
}

TEST(Quiescence, StopAtFirstNodePreservesPosition) {
  auto position = PositionFactory{}(
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  const auto original_fen = FenFactory{}(position);
  const auto original_hash = position.GetHash();
  auto stop = std::make_shared<std::atomic_bool>(true);
  DepthCondition condition{1, stop};
  Quiescence quiescence{condition};
  bool stopped = false;
  for (int i = 0; i < 4096 && !stopped; ++i) {
    stopped = !quiescence
                   .Search<false>(position, std::numeric_limits<Eval>::min() / 2,
                                  std::numeric_limits<Eval>::max() / 2, 0)
                   .has_value();
    ASSERT_EQ(FenFactory{}(position), original_fen);
    ASSERT_EQ(position.GetHash(), original_hash);
  }
  EXPECT_TRUE(stopped);
  EXPECT_EQ(quiescence.GetSearchedNodes(), 1u);
}
TEST(TranspositionTable, IllegalStoredMoveIsIgnored) {
  auto position = PositionFactory{}();
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  table->SetEntry(position, MoveFactory{}(position, "a1a8"), 0, 20,
                  Bound::kExact, Age{});
  Searcher searcher(position, table);
  searcher.InitStartOfSearch();
  DepthCondition condition{3};

  const auto result = searcher.Search<NodeType::kPV>(
      condition, 3, 3, std::numeric_limits<Eval>::min() / 2,
      std::numeric_limits<Eval>::max() / 2);

  ASSERT_TRUE(result.has_value());
  const auto legal_moves =
      MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(position);
  EXPECT_NE(std::ranges::find(legal_moves, searcher.GetCurrentBestMove()),
            legal_moves.end());
}
}  // namespace PositionTests
