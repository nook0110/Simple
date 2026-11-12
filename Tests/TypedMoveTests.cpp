#include <gtest/gtest.h>

#include <array>
#include <type_traits>

#include "MoveFactory.h"
#include "MoveGenerator.h"
#include "MovePicker.h"
#include "PositionFactory.h"
#include "ScopedMove.h"
#include "TypedMove.h"

using namespace SimpleChessEngine;

template <class M>
concept CanDoMove = requires(Position& position, M move) {
  position.DoMove(move);
};

template <class M>
concept CanUndoMove = requires(Position& position, M move) {
  position.UndoMove(move, position.GetIrreversibleData());
};

template <class M>
concept CanConstructLegalMove = requires(M move) {
  LegalMove(move);
};

static_assert(ReversibleMove<LegalMove>);
static_assert(ReversibleMove<NullMove>);
static_assert(!ReversibleMove<Move>);
static_assert(!ReversibleMove<PseudoLegalMove>);
static_assert(!ReversibleMove<MaybeLegalMove>);
static_assert(!ReversibleMove<MaybePseudoLegalMove>);
static_assert(CanDoMove<LegalMove>);
static_assert(CanUndoMove<LegalMove>);
static_assert(!CanDoMove<Move>);
static_assert(!CanDoMove<PseudoLegalMove>);
static_assert(!CanDoMove<MaybeLegalMove>);
static_assert(!CanUndoMove<Move>);
static_assert(!CanUndoMove<PseudoLegalMove>);
static_assert(!CanUndoMove<MaybeLegalMove>);
static_assert(std::is_same_v<decltype(std::declval<MovePicker&>().SelectNextMove()), MaybeLegalMove>);
static_assert(std::is_same_v<
              decltype(std::declval<MovePicker&>().SelectProbCutMove(Eval{})),
              MaybeLegalMove>);
static_assert(!CanConstructLegalMove<Move>);
static_assert(!CanConstructLegalMove<PseudoLegalMove>);
static_assert(!std::is_constructible_v<MaybeLegalMove, Move>);
static_assert(!std::is_constructible_v<MaybePseudoLegalMove, Move>);
static_assert(!static_cast<bool>(MaybeLegalMove::None()));
static_assert(!static_cast<bool>(MaybePseudoLegalMove::None()));
static_assert(sizeof(PseudoLegalMove) == sizeof(Move));
static_assert(sizeof(LegalMove) == sizeof(Move));
static_assert(alignof(PseudoLegalMove) == alignof(Move));
static_assert(alignof(LegalMove) == alignof(Move));
static_assert(std::is_trivially_copyable_v<PseudoLegalMove>);
static_assert(std::is_trivially_copyable_v<LegalMove>);
static_assert(std::is_trivially_default_constructible_v<PseudoLegalMove>);
static_assert(std::is_trivially_default_constructible_v<LegalMove>);
static_assert(std::is_standard_layout_v<PseudoLegalMove>);
static_assert(std::is_standard_layout_v<LegalMove>);
static_assert(std::is_convertible_v<LegalMove, PseudoLegalMove>);
static_assert(!std::is_constructible_v<LegalMove, PseudoLegalMove>);
static_assert(!std::is_constructible_v<LegalMove, Move>);
static_assert(!std::is_constructible_v<PseudoLegalMove, Move>);

TEST(TypedMoves, RejectsGarbage) {
  auto position = PositionFactory{}();
  EXPECT_FALSE(position.TryPseudoLegal(Move(0xffff)));
  EXPECT_FALSE(position.TryPseudoLegal(Move::None()));
  EXPECT_FALSE(position.TryPseudoLegal(Move::Null()));
  EXPECT_FALSE(position.TryPseudoLegal(MoveFactory{}(position, "a1a8")));
}

TEST(TypedMoves, RejectsExposingKing) {
  auto position = PositionFactory{}("4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1");
  const auto pseudo = position.TryPseudoLegal(MoveFactory{}(position, "e2f2"));
  ASSERT_TRUE(pseudo);
  EXPECT_FALSE(position.TryLegal(*pseudo));
  EXPECT_FALSE(position.TryLegal(*pseudo, position.GetIrreversibleData().blockers[0]));
}

TEST(TypedMoves, AcceptsEveryGeneratedLegalMove) {
  for (const auto* fen : {
           "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
           "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
           "4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1",
           "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
           "4k3/8/8/8/8/8/8/6KR w H - 0 1"}) {
    auto position = PositionFactory{}(fen);
    const auto moves = MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(position);
    const auto raw_moves = MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
    ASSERT_EQ(moves.size(), raw_moves.size());
    for (size_t i = 0; i < moves.size(); ++i) EXPECT_EQ(moves[i], raw_moves[i]);
    for (const auto move : moves) {
      const Move raw = move;
      const PseudoLegalMove degraded = move;
      EXPECT_EQ(degraded, raw);
      EXPECT_EQ(raw, degraded);
      EXPECT_EQ(move, raw);
      EXPECT_EQ(raw, move);
      EXPECT_EQ(move, degraded);
      const auto pseudo = position.TryPseudoLegal(raw);
      ASSERT_TRUE(pseudo);
      EXPECT_EQ(*pseudo, degraded);
      const auto legal = position.TryLegal(*pseudo);
      ASSERT_TRUE(legal);
      EXPECT_EQ(*legal, move);
      const auto explicit_blockers = position.TryLegal(
          *pseudo, position.GetIrreversibleData().blockers[static_cast<size_t>(position.GetSideToMove())]);
      ASSERT_TRUE(explicit_blockers);
      EXPECT_EQ(*explicit_blockers, move);
    }
  }
}
