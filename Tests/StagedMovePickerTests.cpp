#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <set>

#include "MoveGenerator.h"
#include "MovePicker.h"
#include "PositionFactory.h"
#include "Searcher.h"

using namespace SimpleChessEngine;

namespace {
constexpr std::array kPositions = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1",
    "4k3/8/8/3pP3/4K3/8/8/8 w - d6 0 1",
    "4k3/8/8/3pP3/8/8/8/4K2r w - d6 0 1",
    "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
    "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1",
    "4k3/8/8/8/1b6/8/4r3/4K3 w - - 0 1",
    "7k/5K1b/7Q/8/8/8/8/8 b - - 0 1",
    "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1",
    "4k3/8/8/8/8/8/8/rRK5 w B - 0 1",
    "4k3/8/8/8/8/8/8/6KR w H - 0 1"};

std::set<std::uint16_t> LegalSet(Position& position) {
  std::set<std::uint16_t> result;
  for (auto move :
       MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position)) {
    result.insert(move.Raw());
  }
  return result;
}
}

TEST(StagedMovePicker, PseudoLegalValidationMatchesGenerator) {
  for (const auto* fen : kPositions) {
    SCOPED_TRACE(fen);
    auto position = PositionFactory{}(fen);
    std::array<Move, MoveGenerator::kMaxMovesPerPosition> generated;
    auto* end = MoveGenerator{}.GeneratePseudoLegal<MoveGenerator::Type::kAll>(
        position, generated.data());
    std::set<std::uint16_t> expected;
    for (auto* p = generated.data(); p != end; ++p) expected.insert(p->Raw());
    for (unsigned raw = 0; raw < 65536; ++raw) {
      EXPECT_EQ(position.IsPseudoLegal(Move(static_cast<std::uint16_t>(raw))),
                expected.contains(raw))
          << raw;
    }
  }
}

TEST(StagedMovePicker, CaptureAndQuietStagesPartitionLegalMoves) {
  for (const auto* fen : kPositions) {
    SCOPED_TRACE(fen);
    auto position = PositionFactory{}(fen);
    const auto expected = LegalSet(position);
    std::array<Move, MoveGenerator::kMaxMovesPerPosition> generated;
    auto* middle =
        MoveGenerator{}.GeneratePseudoLegal<MoveGenerator::Type::kCaptures>(
            position, generated.data());
    auto* end =
        MoveGenerator{}.GeneratePseudoLegal<MoveGenerator::Type::kQuiets>(
            position, middle);
    std::set<std::uint16_t> actual;
    for (auto* p = generated.data(); p != end; ++p) {
      if (!position.IsLegal(*p)) continue;
      EXPECT_TRUE(actual.insert(p->Raw()).second);
    }
    EXPECT_EQ(actual, expected);
  }
}

TEST(StagedMovePicker, MainSearchEmitsLegalMovesOnceWithTtFirst) {
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  for (const auto* fen : kPositions) {
    SCOPED_TRACE(fen);
    auto position = PositionFactory{}(fen);
    const auto uncomputed_data = position.GetIrreversibleData();
    const auto expected = LegalSet(position);
    Searcher searcher(position, table);
    for (const auto tt :
         {Move::None(),
          expected.empty() ? Move::None() : Move(*expected.begin()),
          Move(0xffff)}) {
      MovePicker picker;
      picker.InitPicker(position, &searcher, 0, tt);
      std::set<std::uint16_t> actual;
      auto move = picker.SelectNextMove();
      if (expected.contains(tt.Raw())) {
        EXPECT_EQ(move, tt);
      }
      for (; move.IsValid(); move = picker.SelectNextMove()) {
        ASSERT_TRUE(expected.contains(move.Raw()));
        EXPECT_TRUE(actual.insert(move.Raw()).second);
        position.DoMove(move);
        position.ComputePins(position.GetSideToMove());
        position.UndoMove(move, uncomputed_data);
      }
      EXPECT_EQ(actual, expected);
      EXPECT_EQ(picker.MoveCount(), expected.size());
    }
  }
}

TEST(StagedMovePicker, QuiescencePreservesGeneratorOrderAndEvasions) {
  for (const auto* fen : kPositions) {
    SCOPED_TRACE(fen);
    auto position = PositionFactory{}(fen);
    const auto expected =
        position.IsUnderCheck()
            ? MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position)
            : MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kQuiescence>(
                  position);
    MovePicker picker;
    picker.InitPicker(position, nullptr, 0);
    for (auto move : expected) EXPECT_EQ(picker.SelectNextMove(), move);
    EXPECT_EQ(picker.SelectNextMove(), Move::None());
  }
}
