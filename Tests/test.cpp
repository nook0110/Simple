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
[[nodiscard]] size_t CountPossibleGames(Position& position, const size_t depth)
{
  if (depth == 0) return 1;

  constexpr MoveGenerator generator{};

  size_t answer{};

  const auto moves = generator(position);

  if (moves.empty()) return 1;

  if (depth == 1) return moves.size();

  for (const auto& move : moves)
  {
    position.DoMove(move);
    answer += CountPossibleGames(position, depth - 1);
    position.UndoMove(move);
  }

  return answer;
}

TEST(GenerateMoves, ShannonNumberCheck)
{
  auto start_position = PositionFactory{}();

  constexpr size_t max_plies = 6;

  constexpr std::array<size_t, max_plies + 1> shannon_number = {
      1, 20, 400, 8902, 197281, 4865609, 119060324};

  for (size_t depth = 0; depth <= max_plies; ++depth)
  {
    auto possible_games = CountPossibleGames(start_position, depth);

    ASSERT_EQ(start_position, PositionFactory{}());
    ASSERT_EQ(possible_games, shannon_number[depth]);
  }
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