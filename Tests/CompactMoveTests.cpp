#include <gtest/gtest.h>

#include "Move.h"

using namespace SimpleChessEngine;

namespace CompactMoveTests {

TEST(BasicConstruction, FromToSquares) {
  const Move move(0, 8);
  ASSERT_EQ(move.From(), 0);
  ASSERT_EQ(move.To(), 8);
  ASSERT_TRUE(move.IsNormal());
  ASSERT_TRUE(move.IsValid());
}

TEST(PromotionMoves, AllPieces) {
  const Move queen_promo =
      Move::Make<MoveType::kPromotion>(48, 56, Piece::kQueen);
  ASSERT_TRUE(queen_promo.IsPromotion());
  ASSERT_EQ(queen_promo.PromotionPiece(), Piece::kQueen);

  const Move knight_promo =
      Move::Make<MoveType::kPromotion>(48, 56, Piece::kKnight);
  ASSERT_EQ(knight_promo.PromotionPiece(), Piece::kKnight);

  const Move rook_promo =
      Move::Make<MoveType::kPromotion>(48, 56, Piece::kRook);
  ASSERT_EQ(rook_promo.PromotionPiece(), Piece::kRook);

  const Move bishop_promo =
      Move::Make<MoveType::kPromotion>(48, 56, Piece::kBishop);
  ASSERT_EQ(bishop_promo.PromotionPiece(), Piece::kBishop);
}

TEST(SpecialMoves, EnPassantAndCastling) {
  const Move en_passant = Move::Make<MoveType::kEnPassant>(32, 41);
  ASSERT_TRUE(en_passant.IsEnPassant());
  ASSERT_FALSE(en_passant.IsPromotion());
  ASSERT_FALSE(en_passant.IsCastling());
  ASSERT_FALSE(en_passant.IsNormal());

  const Move castling = Move::Make<MoveType::kCastling>(4, 6);
  ASSERT_TRUE(castling.IsCastling());
  ASSERT_FALSE(castling.IsEnPassant());
  ASSERT_FALSE(castling.IsPromotion());
  ASSERT_FALSE(castling.IsNormal());
}

TEST(MoveEquality, SameAndDifferent) {
  const Move move1(8, 16);
  const Move move2(8, 16);
  const Move move3(8, 24);

  ASSERT_EQ(move1, move2);
  ASSERT_NE(move1, move3);
}

TEST(SpecialValues, NullAndNone) {
  ASSERT_FALSE(Move::Null().IsValid());
  ASSERT_FALSE(Move::None().IsValid());
  ASSERT_NE(Move::Null(), Move::None());
}

TEST(BitLayout, CorrectEncoding) {
  const Move move(63, 0);
  ASSERT_EQ(move.Raw() & 0x3F, 0);
  ASSERT_EQ((move.Raw() >> 6) & 0x3F, 63);
  ASSERT_EQ((move.Raw() >> 14) & 0x3, 0);
}

TEST(SizeAndAlignment, OptimalLayout) {
  ASSERT_EQ(sizeof(Move), 2);
  ASSERT_EQ(alignof(Move), 2);
  ASSERT_TRUE(std::is_trivially_copyable_v<Move>);
  ASSERT_TRUE(std::is_standard_layout_v<Move>);
}

TEST(BooleanConversion, ValidityCheck) {
  const Move valid_move(8, 16);
  const Move none_move = Move::None();

  ASSERT_TRUE(static_cast<bool>(valid_move));
  ASSERT_FALSE(static_cast<bool>(none_move));
}

TEST(RawDataAccess, RoundTrip) {
  const Move move(8, 16);
  const std::uint16_t raw = move.Raw();
  const Move reconstructed(raw);

  ASSERT_EQ(move, reconstructed);
  ASSERT_EQ(move.From(), reconstructed.From());
  ASSERT_EQ(move.To(), reconstructed.To());
}

TEST(AllSquares, ValidEncoding) {
  for (BitIndex from = 0; from < 64; ++from) {
    for (BitIndex to = 0; to < 64; ++to) {
      if (from != to) {
        const Move move(from, to);
        ASSERT_EQ(move.From(), from);
        ASSERT_EQ(move.To(), to);
        ASSERT_TRUE(move.IsValid());
      }
    }
  }
}
}  // namespace CompactMoveTests