#include "pch.h"

#define SPLIT_PCH
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

namespace PositionTest
{
TEST(DoMove, DoAndUndoEqualZero)
{
  const auto start_pos = PositionFactory{}();
  Position pos = start_pos;

  for (const auto moves = MoveGenerator{}(pos); const auto& move : moves)
  {
    pos.DoMove(move);
    pos.UndoMove(move);

    ASSERT_EQ(pos, start_pos);
  }
}
}  // namespace PositionTest

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
    const Position& position,
    const std::chrono::milliseconds left_time = std::chrono::seconds(5))
{
  ChessEngine engine;
  engine.SetPosition(position);

  engine.ComputeBestMove(left_time);

  return engine.GetCurrentBestMove();
}

Move ComputeBestMoveByDepth(const Position& position, const size_t depth = 20)
{
  ChessEngine engine;
  engine.SetPosition(position);

  engine.ComputeBestMove(depth);

  return engine.GetCurrentBestMove();
}

class BestMoveTest : public testing::TestWithParam<std::string>
{
 protected:
  [[nodiscard]] Position GeneratePosition() const
  {
    const auto fen = GetFen();
    return PositionFactory{}(fen);
  }

  [[nodiscard]] Move GetAnswer() const
  {
    const auto move = GetMove();
    return MoveFactory{}(move);
  }

 private:
  [[nodiscard]] std::string GetFen() const
  {
    const auto& epd = GetParam();
    static const std::string best_move_word = "bm";

    const auto end_of_fen = epd.find(best_move_word);

    return epd.substr(0, end_of_fen);
  }

  [[nodiscard]] std::string GetMove() const
  {
    const auto& epd = GetParam();
    static const std::string best_move_word = "bm";

    const auto end_of_fen = epd.find(best_move_word);

    static const std::string best_move_end = ";";
    const auto end_of_move = epd.find(best_move_end);

    const auto move_start = end_of_fen + best_move_word.size();

    return epd.substr(move_start + 1, end_of_move - move_start);
  }
};

TEST_P(BestMoveTest, FindBestMove)
{
  const auto position = GeneratePosition();

  const auto first_answer = ComputeBestMoveByTime(position);
  const auto second_answer = ComputeBestMoveByDepth(position);

  ASSERT_EQ(first_answer, GetAnswer());
  ASSERT_EQ(second_answer, GetAnswer());
}

INSTANTIATE_TEST_CASE_P(
    Name, BestMoveTest,
    testing::Values(
        R"(1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - - bm Qd1+; id "BK.01")"));
}  // namespace BestMoveTests