// ReSharper disable CppClangTidyBugproneSuspiciousInclude
#include "pch.h"

// WARNING! pch.h must be first header!

#include "../Chess/Attacks.cpp"
#include "../Chess/Attacks.h"
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
TEST(GetFirstBit, NoBits)
{
  // all bits are 0
  constexpr Bitboard bitboard;

  // should be nullopt
  ASSERT_FALSE(bitboard.GetFirstBit());
}

TEST(GetFirstBit, AllBits)
{
  Bitboard bitboard;
  // all bits are 1
  bitboard.Set();

  // should not be nullopt
  ASSERT_TRUE(bitboard.GetFirstBit());

  // should be first (0-index) bit
  ASSERT_EQ(bitboard.GetFirstBit().value(), 0);
}

TEST(GetFirstBit, EachBit)
{
  for (size_t i = 0; i < kBitboardSize; ++i)
  {
    Bitboard bitboard;
    // all bits are 0, except i
    bitboard.Set(i);

    // should not be nullopt
    ASSERT_TRUE(bitboard.GetFirstBit());
    // should be i
    ASSERT_EQ(bitboard.GetFirstBit().value(), i);
  }
}
}  // namespace BitBoardTests

namespace AttackMapTests
{
struct TestCase
{
  Bitboard occupancy;
  BitIndex square;
  Bitboard answer;
};

template <Piece sliding_piece>
class TestAttackMaskGeneration : public testing::TestWithParam<TestCase>
{
 protected:
  [[nodiscard]] Bitboard GetMask() const
  {
    auto test_case = GetParam();
    return GenerateAttackMask<sliding_piece>(test_case.square,
                                             test_case.occupancy);
  }
  [[nodiscard]] Bitboard GetAnswer() const { return GetParam().answer; }
};

using BishopMaskTest = TestAttackMaskGeneration<Piece::kBishop>;
using RookMaskTest = TestAttackMaskGeneration<Piece::kRook>;

TEST_P(BishopMaskTest, AttackMaskGeneration)
{
  ASSERT_EQ(GetMask(), GetAnswer());
}

TEST_P(RookMaskTest, AttackMaskGeneration)
{
  ASSERT_EQ(GetMask(), GetAnswer());
}

INSTANTIATE_TEST_CASE_P(
    RandomBoardBishop, BishopMaskTest,
    testing::Values(
        TestCase{Bitboard{0x40c601f030da9200}, 52,
                 Bitboard{0x2800284402010000}},
        TestCase{Bitboard{0x20346a0215120000}, 19, Bitboard{0x14001422}},
        TestCase{Bitboard{0x600120400c222288}, 53,
                 Bitboard{0x5000508804000000}},
        TestCase{Bitboard{0xc240904011582201}, 4, Bitboard{0x102042800}},
        TestCase{Bitboard{0x4c548408443a400}, 44, Bitboard{0x4428002844800000}},
        TestCase{Bitboard{0x1a529494440060f}, 17, Bitboard{0x5000500}},
        TestCase{Bitboard{0x1062002008502088}, 55,
                 Bitboard{0x4000402000000000}},
        TestCase{Bitboard{0x12420d18100c0642}, 12, Bitboard{0x8040280028}},
        TestCase{Bitboard{0x204c1808020c20a}, 52, Bitboard{0x2800284482010000}},
        TestCase{Bitboard{0xa0010290c120e}, 56, Bitboard{0x2000000000000}},
        TestCase{Bitboard{0xaa5450937910031}, 1, Bitboard{0x10080500}},
        TestCase{Bitboard{0x208a011a4424150}, 40, Bitboard{0x402000204000000}},
        TestCase{Bitboard{0x189a000194003820}, 56, Bitboard{0x2000000000000}},
        TestCase{Bitboard{0x40103129091431}, 17, Bitboard{0x100805000500}},
        TestCase{Bitboard{0x86140949401340a}, 43, Bitboard{0x2214001400000000}},
        TestCase{Bitboard{0x9c090a544c280040}, 42, Bitboard{0x10a000a11200000}},
        TestCase{Bitboard{0x21010a151a405350}, 44,
                 Bitboard{0x4428002844820100}},
        TestCase{Bitboard{0x4831b41488027311}, 3, Bitboard{0x21400}},
        TestCase{Bitboard{0x1012012370c1800}, 0, Bitboard{0x40200}},
        TestCase{Bitboard{0x1226004048909}, 52, Bitboard{0x2800280402010000}},
        TestCase{Bitboard{0x1213089000011c26}, 62, Bitboard{0xa0100804020100}},
        TestCase{Bitboard{0x88e4808200200c0c}, 49, Bitboard{0x500050810200000}},
        TestCase{Bitboard{0x40180e2241e00cc0}, 32, Bitboard{0x20002040800}},
        TestCase{Bitboard{0x8200c0002ca0c488}, 55,
                 Bitboard{0x4000400000000000}},
        TestCase{Bitboard{0x6600e00418318140}, 43,
                 Bitboard{0x2214001420408000}}));

INSTANTIATE_TEST_CASE_P(
    RandomBoardRook, RookMaskTest,
    testing::Values(
        TestCase{Bitboard{0x40c601f030da9200}, 52,
                 Bitboard{0x106c101000000000}},
        TestCase{Bitboard{0x20346a0215120000}, 19, Bitboard{0x80808160808}},
        TestCase{Bitboard{0x600120400c222288}, 53,
                 Bitboard{0x20df200000000000}},
        TestCase{Bitboard{0xc240904011582201}, 4, Bitboard{0x1010ef}},
        TestCase{Bitboard{0x4c548408443a400}, 44, Bitboard{0x1010681010101010}},
        TestCase{Bitboard{0x1a529494440060f}, 17, Bitboard{0x2020202027d0200}},
        TestCase{Bitboard{0x1062002008502088}, 55,
                 Bitboard{0x8040808080808080}},
        TestCase{Bitboard{0x12420d18100c0642}, 12, Bitboard{0x1010ec10}},
        TestCase{Bitboard{0x204c1808020c20a}, 52, Bitboard{0x10ec101010101010}},
        TestCase{Bitboard{0xa0010290c120e}, 56, Bitboard{0xfe01010101000000}},
        TestCase{Bitboard{0xaa5450937910031}, 1, Bitboard{0x202021d}},
        TestCase{Bitboard{0x208a011a4424150}, 40, Bitboard{0x1013e0100000000}},
        TestCase{Bitboard{0x189a000194003820}, 56, Bitboard{0xe01010100000000}},
        TestCase{Bitboard{0x40103129091431}, 17, Bitboard{0x2020202020d0202}},
        TestCase{Bitboard{0x86140949401340a}, 43, Bitboard{0x808770808080808}},
        TestCase{Bitboard{0x9c090a544c280040}, 42, Bitboard{0x4040a0400000000}},
        TestCase{Bitboard{0x21010a151a405350}, 44,
                 Bitboard{0x1010e81000000000}},
        TestCase{Bitboard{0x4831b41488027311}, 3, Bitboard{0x8080817}},
        TestCase{Bitboard{0x1012012370c1800}, 0, Bitboard{0x10101fe}},
        TestCase{Bitboard{0x1226004048909}, 52, Bitboard{0x10ef101010101010}},
        TestCase{Bitboard{0x1213089000011c26}, 62,
                 Bitboard{0xb040404040404040}},
        TestCase{Bitboard{0x88e4808200200c0c}, 49, Bitboard{0x205020200000000}},
        TestCase{Bitboard{0x40180e2241e00cc0}, 32, Bitboard{0x101010201000000}},
        TestCase{Bitboard{0x8200c0002ca0c488}, 55,
                 Bitboard{0x807f800000000000}},
        TestCase{Bitboard{0x6600e00418318140}, 43,
                 Bitboard{0x808370808000000}}));

struct TestCaseWithoutAnswer
{
  Bitboard occupancy;
  BitIndex square;
};

template <Piece sliding_piece>
class TestAttackMapTable : public testing::TestWithParam<TestCaseWithoutAnswer>
{
 protected:
  [[nodiscard]] Bitboard GetMask() const
  {
    auto test_case = GetParam();
    return AttackTable<sliding_piece>::GetAttackMap(test_case.square,
                                                    test_case.occupancy);
  }
  [[nodiscard]] Bitboard GetAnswer() const
  {
    auto test_case = GetParam();
    return GenerateAttackMask<sliding_piece>(test_case.square,
                                             test_case.occupancy);
  }
};

using BishopAttackMapTest = TestAttackMapTable<Piece::kBishop>;
using RookAttackMapTest = TestAttackMapTable<Piece::kRook>;

TEST_P(BishopAttackMapTest, AttackMap) { ASSERT_EQ(GetMask(), GetAnswer()); }

TEST_P(RookAttackMapTest, AttackMap) { ASSERT_EQ(GetMask(), GetAnswer()); }

INSTANTIATE_TEST_CASE_P(
    RandomBoardBishop, BishopAttackMapTest,
    testing::Values(TestCaseWithoutAnswer{Bitboard{0x40c601f030da9200}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x20346a0215120000}, 19},
                    TestCaseWithoutAnswer{Bitboard{0x600120400c222288}, 53},
                    TestCaseWithoutAnswer{Bitboard{0xc240904011582201}, 4},
                    TestCaseWithoutAnswer{Bitboard{0x4c548408443a400}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x1a529494440060f}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x1062002008502088}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x12420d18100c0642}, 12},
                    TestCaseWithoutAnswer{Bitboard{0x204c1808020c20a}, 52},
                    TestCaseWithoutAnswer{Bitboard{0xa0010290c120e}, 56},
                    TestCaseWithoutAnswer{Bitboard{0xaa5450937910031}, 1},
                    TestCaseWithoutAnswer{Bitboard{0x208a011a4424150}, 40},
                    TestCaseWithoutAnswer{Bitboard{0x189a000194003820}, 56},
                    TestCaseWithoutAnswer{Bitboard{0x40103129091431}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x86140949401340a}, 43},
                    TestCaseWithoutAnswer{Bitboard{0x9c090a544c280040}, 42},
                    TestCaseWithoutAnswer{Bitboard{0x21010a151a405350}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x4831b41488027311}, 3},
                    TestCaseWithoutAnswer{Bitboard{0x1012012370c1800}, 0},
                    TestCaseWithoutAnswer{Bitboard{0x1226004048909}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x1213089000011c26}, 62},
                    TestCaseWithoutAnswer{Bitboard{0x88e4808200200c0c}, 49},
                    TestCaseWithoutAnswer{Bitboard{0x40180e2241e00cc0}, 32},
                    TestCaseWithoutAnswer{Bitboard{0x8200c0002ca0c488}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x6600e00418318140}, 43}));

INSTANTIATE_TEST_CASE_P(
    RandomBoardRook, RookAttackMapTest,
    testing::Values(TestCaseWithoutAnswer{Bitboard{0x40c601f030da9200}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x20346a0215120000}, 19},
                    TestCaseWithoutAnswer{Bitboard{0x600120400c222288}, 53},
                    TestCaseWithoutAnswer{Bitboard{0xc240904011582201}, 4},
                    TestCaseWithoutAnswer{Bitboard{0x4c548408443a400}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x1a529494440060f}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x1062002008502088}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x12420d18100c0642}, 12},
                    TestCaseWithoutAnswer{Bitboard{0x204c1808020c20a}, 52},
                    TestCaseWithoutAnswer{Bitboard{0xa0010290c120e}, 56},
                    TestCaseWithoutAnswer{Bitboard{0xaa5450937910031}, 1},
                    TestCaseWithoutAnswer{Bitboard{0x208a011a4424150}, 40},
                    TestCaseWithoutAnswer{Bitboard{0x189a000194003820}, 56},
                    TestCaseWithoutAnswer{Bitboard{0x40103129091431}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x86140949401340a}, 43},
                    TestCaseWithoutAnswer{Bitboard{0x9c090a544c280040}, 42},
                    TestCaseWithoutAnswer{Bitboard{0x21010a151a405350}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x4831b41488027311}, 3},
                    TestCaseWithoutAnswer{Bitboard{0x1012012370c1800}, 0},
                    TestCaseWithoutAnswer{Bitboard{0x1226004048909}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x1213089000011c26}, 62},
                    TestCaseWithoutAnswer{Bitboard{0x88e4808200200c0c}, 49},
                    TestCaseWithoutAnswer{Bitboard{0x40180e2241e00cc0}, 32},
                    TestCaseWithoutAnswer{Bitboard{0x8200c0002ca0c488}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x6600e00418318140}, 43}));
}  // namespace AttackMapTests

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