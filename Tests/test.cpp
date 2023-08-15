#include "pch.h"

#define SplitPCH
#include "../Chess/BitBoard.h"
#include "../Chess/MoveGenerator.h"

using namespace SimpleChessEngine;

namespace BitBoardTests
{
constexpr auto kDefaultBitboardSize = 64;
TEST(GetFirstBit, NoBits)
{
  // all bits are 0
  Bitboard<kDefaultBitboardSize> x;

  // should be nullopt
  ASSERT_FALSE(x.GetFirstBit());
}

TEST(GetFirstBit, AllBits)
{
  Bitboard<kDefaultBitboardSize> x;
  // all bits are 1
  x.set();

  // should not be nullopt
  ASSERT_TRUE(x.GetFirstBit());

  // should be first (0-index) bit
  ASSERT_EQ(x.GetFirstBit().value(), 0);
}

TEST(GetFirstBit, EachBit)
{
  for (int i = 0; i < kDefaultBitboardSize; ++i)
  {
    Bitboard<64> x;
    // all bits are 0, except i
    x.set(i);

    // should not be nullopt
    ASSERT_TRUE(x.GetFirstBit());
    // should be i
    ASSERT_EQ(x.GetFirstBit().value(), i);
  }
}
}  // namespace BitBoardTests

namespace my_namespace
{}