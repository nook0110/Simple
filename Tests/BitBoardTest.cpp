#include <gtest/gtest.h>

#include "Chess/BitBoard.h"
#include "Chess/Utility.h"

using namespace SimpleChessEngine;

namespace BitBoardTests {
TEST(GetFirstBit, NoBits) {
  // all bits are 0
  constexpr Bitboard bitboard;

  // should be nullopt
  ASSERT_FALSE(bitboard.Any());
}

TEST(GetFirstBit, AllBits) {
  Bitboard bitboard;
  // all bits are 1
  bitboard.Set();

  // should not be nullopt
  ASSERT_TRUE(bitboard.Any());

  // should be first (0-index) bit
  ASSERT_EQ(bitboard.GetFirstBit(), 0);
}

TEST(GetFirstBit, EachBit) {
  for (size_t i = 0; i < kBoardArea; ++i) {
    Bitboard bitboard;
    // all bits are 0, except i
    bitboard.Set(i);

    // should not be nullopt
    ASSERT_TRUE(bitboard.Any());
    // should be i
    ASSERT_EQ(bitboard.GetFirstBit(), i);
  }
}
} // namespace BitBoardTests