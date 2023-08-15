#include "pch.h"

#define SplitPCH
#include "../Chess/Attacks.cpp"
#include "../Chess/BitBoard.h"
#include "../Chess/MoveFactory.h"
#include "../Chess/MoveGenerator.cpp"
#include "../Chess/MoveGenerator.h"
#include "../Chess/Position.cpp"
#include "../Chess/PositionFactory.h"
#include "../Chess/SimpleChessEngine.h"

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

namespace MoveGeneratorTests
{
TEST(GenerateMoves, StartPosition)
{
  auto start_position = PositionFactory{}();

  constexpr MoveGenerator generator{};

  const auto moves = generator(start_position);

  // ASSERT_EQ(start_position, PositionFactory{}());

  ASSERT_EQ(moves.size(), static_cast<size_t>(20));
}
}  // namespace MoveGeneratorTests

namespace BestMoveTests
{
Move ComputeBestMoveByTime(
    Position position,
    const std::chrono::milliseconds left_time = std::chrono::seconds(5))
{
  ChessEngine engine;
  engine.SetPosition(std::move(position));

  engine.ComputeBestMove(left_time);

  return engine.GetCurrentBestMove();
}

Move ComputeBestMoveByDepth(Position position, const size_t depth = 20)
{
  ChessEngine engine;
  engine.SetPosition(std::move(position));

  engine.ComputeBestMove(depth);

  return engine.GetCurrentBestMove();
}
}  // namespace BestMoveTests