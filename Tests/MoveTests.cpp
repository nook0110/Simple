#include <gtest/gtest.h>
#include "../Chess/Move.h"
#include <vector>
#include <algorithm>

using namespace SimpleChessEngine;

// Test basic Move functionality
TEST(MoveTest, BasicConstruction) {
  Move move1(0, 1);
  EXPECT_EQ(move1.From(), 0);
  EXPECT_EQ(move1.To(), 1);
  EXPECT_TRUE(move1.IsValid());
  
  Move move2 = Move::None();
  EXPECT_FALSE(move2.IsValid());
  
  Move move3 = Move::Null();
  EXPECT_FALSE(move3.IsValid());
}

TEST(MoveTest, MoveTypes) {
  auto normal = Move(0, 1);
  EXPECT_TRUE(normal.IsNormal());
  EXPECT_FALSE(normal.IsPromotion());
  EXPECT_FALSE(normal.IsEnPassant());
  EXPECT_FALSE(normal.IsCastling());
  
  auto promotion = Move::Make<MoveType::kPromotion>(0, 8, Piece::kQueen);
  EXPECT_TRUE(promotion.IsPromotion());
  EXPECT_EQ(promotion.PromotionPiece(), Piece::kQueen);
  
  auto enpassant = Move::Make<MoveType::kEnPassant>(0, 8);
  EXPECT_TRUE(enpassant.IsEnPassant());
  
  auto castling = Move::Make<MoveType::kCastling>(4, 6);
  EXPECT_TRUE(castling.IsCastling());
}

// Test TypedMove value types
TEST(TypedMoveTest, ValueTypeConstruction) {
  Move move(0, 1);
  
  PseudoLegalMove pseudo = MakeTypedMove<PseudoLegalTag>(move);
  EXPECT_EQ(pseudo.From(), 0);
  EXPECT_EQ(pseudo.To(), 1);
  
  LegalMove legal = MakeTypedMove<LegalTag>(move);
  EXPECT_EQ(legal.From(), 0);
  EXPECT_EQ(legal.To(), 1);
}

TEST(TypedMoveTest, ReferenceTypeConstruction) {
  Move move(0, 1);
  
  PseudoLegalMoveRef pseudo_ref = MakeTypedMove<PseudoLegalTag, Move&>(move);
  EXPECT_EQ(pseudo_ref.From(), 0);
  EXPECT_EQ(pseudo_ref.To(), 1);
  
  PseudoLegalMoveConstRef pseudo_const_ref = MakeTypedMove<PseudoLegalTag, const Move&>(move);
  EXPECT_EQ(pseudo_const_ref.From(), 0);
  EXPECT_EQ(pseudo_const_ref.To(), 1);
  
  LegalMoveRef legal_ref = MakeTypedMove<LegalTag, Move&>(move);
  EXPECT_EQ(legal_ref.From(), 0);
  EXPECT_EQ(legal_ref.To(), 1);
  
  LegalMoveConstRef legal_const_ref = MakeTypedMove<LegalTag, const Move&>(move);
  EXPECT_EQ(legal_const_ref.From(), 0);
  EXPECT_EQ(legal_const_ref.To(), 1);
}

// Test conversions between typed moves
TEST(TypedMoveTest, ValueToReferenceConversion) {
  PseudoLegalMove pseudo_value = MakeTypedMove<PseudoLegalTag>(Move(0, 1));
  
  // Value to non-const reference
  PseudoLegalMoveRef pseudo_ref = pseudo_value;
  EXPECT_EQ(pseudo_ref.From(), 0);
  EXPECT_EQ(pseudo_ref.To(), 1);
  
  // Value to const reference
  PseudoLegalMoveConstRef pseudo_const_ref = pseudo_value;
  EXPECT_EQ(pseudo_const_ref.From(), 0);
  EXPECT_EQ(pseudo_const_ref.To(), 1);
  
  LegalMove legal_value = MakeTypedMove<LegalTag>(Move(2, 3));
  
  // Value to non-const reference
  LegalMoveRef legal_ref = legal_value;
  EXPECT_EQ(legal_ref.From(), 2);
  EXPECT_EQ(legal_ref.To(), 3);
  
  // Value to const reference
  LegalMoveConstRef legal_const_ref = legal_value;
  EXPECT_EQ(legal_const_ref.From(), 2);
  EXPECT_EQ(legal_const_ref.To(), 3);
}

TEST(TypedMoveTest, ReferenceToValueConversion) {
  Move move(0, 1);
  PseudoLegalMoveRef pseudo_ref = MakeTypedMove<PseudoLegalTag, Move&>(move);
  
  // Reference to value
  PseudoLegalMove pseudo_value = pseudo_ref;
  EXPECT_EQ(pseudo_value.From(), 0);
  EXPECT_EQ(pseudo_value.To(), 1);
  
  LegalMoveRef legal_ref = MakeTypedMove<LegalTag, Move&>(move);
  
  // Reference to value
  LegalMove legal_value = legal_ref;
  EXPECT_EQ(legal_value.From(), 0);
  EXPECT_EQ(legal_value.To(), 1);
}

TEST(TypedMoveTest, TagConversion) {
  Move move(0, 1);
  LegalMove legal = MakeTypedMove<LegalTag>(move);
  
  // LegalMove can convert to PseudoLegalMove
  PseudoLegalMove pseudo = legal;
  EXPECT_EQ(pseudo.From(), 0);
  EXPECT_EQ(pseudo.To(), 1);
  
  // Both can convert to Move
  Move move1 = legal;
  Move move2 = pseudo;
  EXPECT_EQ(move1, move);
  EXPECT_EQ(move2, move);
}

// Test MoveList
TEST(MoveListTest, BasicOperations) {
  MoveList<PseudoLegalTag> moves;
  
  EXPECT_TRUE(moves.empty());
  EXPECT_EQ(moves.size(), 0);
  
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(2, 3)));
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(4, 5)));
  
  EXPECT_FALSE(moves.empty());
  EXPECT_EQ(moves.size(), 3);
  
  EXPECT_EQ(moves[0].From(), 0);
  EXPECT_EQ(moves[0].To(), 1);
  EXPECT_EQ(moves[1].From(), 2);
  EXPECT_EQ(moves[1].To(), 3);
  EXPECT_EQ(moves[2].From(), 4);
  EXPECT_EQ(moves[2].To(), 5);
}

TEST(MoveListTest, FrontBack) {
  MoveList<LegalTag> moves;
  moves.push_back(MakeTypedMove<LegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(2, 3)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(4, 5)));
  
  EXPECT_EQ(moves.front().From(), 0);
  EXPECT_EQ(moves.front().To(), 1);
  EXPECT_EQ(moves.back().From(), 4);
  EXPECT_EQ(moves.back().To(), 5);
}

TEST(MoveListTest, IteratorBasics) {
  MoveList<PseudoLegalTag> moves;
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(2, 3)));
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(4, 5)));
  
  auto it = moves.begin();
  EXPECT_EQ(it->From(), 0);
  EXPECT_EQ(it->To(), 1);
  
  ++it;
  EXPECT_EQ(it->From(), 2);
  EXPECT_EQ(it->To(), 3);
  
  ++it;
  EXPECT_EQ(it->From(), 4);
  EXPECT_EQ(it->To(), 5);
  
  ++it;
  EXPECT_EQ(it, moves.end());
}

TEST(MoveListTest, ConstIterator) {
  MoveList<LegalTag> moves;
  moves.push_back(MakeTypedMove<LegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(2, 3)));
  
  const auto& const_moves = moves;
  
  auto it = const_moves.begin();
  EXPECT_EQ(it->From(), 0);
  EXPECT_EQ(it->To(), 1);
  
  ++it;
  EXPECT_EQ(it->From(), 2);
  EXPECT_EQ(it->To(), 3);
  
  ++it;
  EXPECT_EQ(it, const_moves.end());
}

TEST(MoveListTest, RangeBasedFor) {
  MoveList<PseudoLegalTag> moves;
  moves.push_back(MakeTypedMove<LegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(2, 3)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(4, 5)));
  
  int count = 0;
  for (const auto& move : moves) {
    EXPECT_TRUE(move.IsValid());
    EXPECT_EQ(move.From(), count * 2);
    EXPECT_EQ(move.To(), count * 2 + 1);
    ++count;
  }
  EXPECT_EQ(count, 3);
}

TEST(MoveListTest, IteratorArithmetic) {
  MoveList<LegalTag> moves;
  for (int i = 0; i < 10; ++i) {
    moves.push_back(MakeTypedMove<LegalTag>(Move(i, i + 1)));
  }
  
  auto it = moves.begin();
  
  // Addition
  auto it2 = it + 5;
  EXPECT_EQ(it2->From(), 5);
  
  // Subtraction
  auto it3 = it2 - 3;
  EXPECT_EQ(it3->From(), 2);
  
  // Difference
  EXPECT_EQ(it2 - it, 5);
  EXPECT_EQ(it3 - it, 2);
  
  // Subscript
  EXPECT_EQ(it[0].From(), 0);
  EXPECT_EQ(it[5].From(), 5);
  EXPECT_EQ(it[9].From(), 9);
}

TEST(MoveListTest, IteratorComparison) {
  MoveList<PseudoLegalTag> moves;
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(2, 3)));
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(4, 5)));
  
  auto it1 = moves.begin();
  auto it2 = moves.begin();
  auto it3 = moves.begin() + 1;
  auto end = moves.end();
  
  EXPECT_TRUE(it1 == it2);
  EXPECT_FALSE(it1 != it2);
  EXPECT_TRUE(it1 != it3);
  EXPECT_FALSE(it1 == it3);
  
  EXPECT_TRUE(it1 < it3);
  EXPECT_TRUE(it1 <= it3);
  EXPECT_FALSE(it1 > it3);
  EXPECT_FALSE(it1 >= it3);
  
  EXPECT_TRUE(it3 > it1);
  EXPECT_TRUE(it3 >= it1);
  EXPECT_FALSE(it3 < it1);
  EXPECT_FALSE(it3 <= it1);
  
  EXPECT_TRUE(it1 < end);
  EXPECT_TRUE(it3 < end);
}

TEST(MoveListTest, STLAlgorithms) {
  MoveList<LegalTag> moves;
  moves.push_back(MakeTypedMove<LegalTag>(Move(5, 6)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(1, 2)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(3, 4)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(7, 8)));
  
  // Find
  auto it = std::find_if(moves.begin(), moves.end(), 
                         [](const auto& m) { return m.From() == 3; });
  EXPECT_NE(it, moves.end());
  EXPECT_EQ(it->From(), 3);
  EXPECT_EQ(it->To(), 4);
  
  // Count
  int count = std::count_if(moves.begin(), moves.end(),
                            [](const auto& m) { return m.From() > 2; });
  EXPECT_EQ(count, 3);
  
  // Sort - need to work with the underlying data
  auto& data = const_cast<std::vector<Move>&>(moves.data());
  std::sort(data.begin(), data.end(),
            [](const Move& a, const Move& b) { return a.From() < b.From(); });
  
  EXPECT_EQ(moves[0].From(), 1);
  EXPECT_EQ(moves[1].From(), 3);
  EXPECT_EQ(moves[2].From(), 5);
  EXPECT_EQ(moves[3].From(), 7);
}

TEST(MoveListTest, EmplaceBack) {
  MoveList<PseudoLegalTag> moves;
  
  auto ref = moves.emplace_back(0, 1);
  EXPECT_EQ(ref.From(), 0);
  EXPECT_EQ(ref.To(), 1);
  EXPECT_EQ(moves.size(), 1);
  
  moves.emplace_back(2, 3);
  EXPECT_EQ(moves.size(), 2);
  EXPECT_EQ(moves[1].From(), 2);
  EXPECT_EQ(moves[1].To(), 3);
}

TEST(MoveListTest, Clear) {
  MoveList<LegalTag> moves;
  moves.push_back(MakeTypedMove<LegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<LegalTag>(Move(2, 3)));
  
  EXPECT_EQ(moves.size(), 2);
  
  moves.clear();
  EXPECT_EQ(moves.size(), 0);
  EXPECT_TRUE(moves.empty());
}

TEST(MoveListTest, Reserve) {
  MoveList<PseudoLegalTag> moves;
  
  moves.reserve(100);
  EXPECT_GE(moves.capacity(), 100);
  EXPECT_EQ(moves.size(), 0);
  
  for (int i = 0; i < 50; ++i) {
    moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(i, i + 1)));
  }
  
  EXPECT_EQ(moves.size(), 50);
  EXPECT_GE(moves.capacity(), 100);
}

// Test iterator value_type
TEST(MoveListTest, IteratorValueType) {
  MoveList<LegalTag> moves;
  moves.push_back(MakeTypedMove<LegalTag>(Move(0, 1)));
  
  using IteratorType = decltype(moves.begin());
  using ValueType = typename IteratorType::value_type;
  
  // value_type should be Move, not TypedMove
  static_assert(std::is_same_v<ValueType, Move>, 
                "Iterator value_type should be Move");
}

// Test that reference types work correctly
TEST(MoveListTest, ReferenceTypeCorrectness) {
  MoveList<PseudoLegalTag> moves;
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(0, 1)));
  moves.push_back(MakeTypedMove<PseudoLegalTag>(Move(2, 3)));
  
  // Non-const iterator should return PseudoLegalMoveRef
  auto it = moves.begin();
  using RefType = decltype(*it);
  static_assert(std::is_same_v<RefType, PseudoLegalMoveRef>,
                "Non-const iterator should return PseudoLegalMoveRef");
  
  // Const iterator should return PseudoLegalMoveConstRef
  const auto& const_moves = moves;
  auto const_it = const_moves.begin();
  using ConstRefType = decltype(*const_it);
  static_assert(std::is_same_v<ConstRefType, PseudoLegalMoveConstRef>,
                "Const iterator should return PseudoLegalMoveConstRef");
}