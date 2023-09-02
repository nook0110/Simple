// ReSharper disable CppClangTidyBugproneSuspiciousInclude
#include "pch.h"

// WARNING! pch.h must be first header!

#include "../Chess/Attacks.cpp"
#include "../Chess/Attacks.h"
#include "../Chess/BitBoard.h"
#include "../Chess/Evaluation.cpp"
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
  for (size_t i = 0; i < kBoardArea; ++i)
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
    const auto [occupancy, square, answer] = GetParam();
    return GenerateAttackMask<sliding_piece>(square, occupancy);
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
    const auto test_case = GetParam();
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

namespace BestMoveTests
{
const Move& ComputeBestMoveByTime(
    const Position& position,
    const std::chrono::milliseconds left_time = std::chrono::seconds(5))
{
  ChessEngine engine;
  engine.SetPosition(position);

  engine.ComputeBestMove(left_time);

  return engine.GetCurrentBestMove();
}

const Move& ComputeBestMoveByDepth(const Position& position,
                                   const size_t depth = 5)
{
  ChessEngine engine;
  engine.SetPosition(position);

  engine.ComputeBestMove(depth);

  return engine.GetCurrentBestMove();
}

struct BestMoveCase
{
  std::string fen;
  std::string bm;
};

class BestMoveTest : public testing::TestWithParam<BestMoveCase>
{
 protected:
  void SetUp() override
  {
    GeneratePosition();
    GenerateAnswer();
  }

  [[nodiscard]] const Position& GetPosition() const { return position_; }

  [[nodiscard]] const Move& GetAnswer() const { return answer_; }

 private:
  void GeneratePosition()
  {
    const auto fen = GetFen();
    position_ = PositionFactory{}(fen);
  }

  void GenerateAnswer()
  {
    const auto move = GetMove();
    answer_ = MoveFactory{}(GetPosition(), move);
  }

  [[nodiscard]] const std::string& GetFen() const { return GetParam().fen; }

  [[nodiscard]] const std::string& GetMove() const { return GetParam().bm; }

  Position position_;
  Move answer_;
};

TEST_P(BestMoveTest, FindBestMove)
{
  const auto& position = GetPosition();

  const auto first_answer =
      ComputeBestMoveByTime(position, std::chrono::seconds(30));
  ASSERT_EQ(first_answer, GetAnswer());

  const auto second_answer = ComputeBestMoveByDepth(position, 7);

  ASSERT_EQ(second_answer, GetAnswer());
}

INSTANTIATE_TEST_CASE_P(
    Name, BestMoveTest,
    testing::Values(BestMoveCase{
        R"(1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -)", "d6d1"}));
}  // namespace BestMoveTests

namespace PositionTest
{
TEST(DoMove, DoAndUndoEqualZero)
{
  const auto start_pos = PositionFactory{}();
  Position pos = start_pos;

  for (const auto moves = MoveGenerator{}.GenerateMoves<false>(pos);
       const auto& move : moves)
  {
    const auto irreversible_data = pos.GetIrreversibleData();
    pos.DoMove(move);
    pos.UndoMove(move, irreversible_data);

    ASSERT_EQ(pos, start_pos);
  }
}
}  // namespace PositionTest

namespace MoveGeneratorTests
{
struct GameInfo
{
  std::optional<size_t> possible_games{};

  std::optional<size_t> en_croissants{};
  std::optional<size_t> castlings{};

  std::optional<size_t> ends_of_game{};

  GameInfo& operator+=(const GameInfo& other)
  {
    possible_games.value() += other.possible_games.value();
    en_croissants.value() += other.en_croissants.value();
    castlings.value() += other.castlings.value();
    ends_of_game.value() += other.ends_of_game.value();
    return *this;
  }
};

[[nodiscard]] GameInfo CountPossibleGames(Position& position,
                                          const size_t depth)
{
  if (depth == 0) return {1, 0, 0, 0};

  GameInfo answer{0, 0, 0, 0};

  const auto moves = MoveGenerator{}.GenerateMoves<false>(position);

  if (depth == 1)
  {
    for (const auto& move : moves)
    {
      if (std::get_if<EnCroissant>(&move))
      {
        answer.en_croissants.value()++;
      }
      if (std::get_if<Castling>(&move))
      {
        answer.castlings.value()++;
      }
    }

    if (moves.empty()) answer.ends_of_game.value() += 1;

    answer.possible_games = moves.size();
    return answer;
  }

  for (const auto& move : moves)
  {
    const auto irreversible_data = position.GetIrreversibleData();
    position.DoMove(move);
    answer += CountPossibleGames(position, depth - 1);
    position.UndoMove(move, irreversible_data);
  }

  return answer;
}

struct GenTestCase
{
  std::string fen;

  std::vector<GameInfo> infos;
};

class GenerateMovesTest : public testing::TestWithParam<GenTestCase>
{
 protected:
  void SetUp() override
  {
    const auto& [fen, infos] = GetParam();

    GeneratePosition();

    infos_ = infos;
  }

  [[nodiscard]] const Position& GetPosition() const { return position_; }

  [[nodiscard]] const GameInfo& GetInfo(const size_t depth) const
  {
    return infos_[depth];
  }

  [[nodiscard]] const size_t GetMaxDepth() const { return infos_.size(); }

 private:
  void GeneratePosition() { position_ = PositionFactory{}(GetParam().fen); }

  std::vector<GameInfo> infos_;
  Position position_;
};

TEST_P(GenerateMovesTest, Perft)
{
  auto position = GetPosition();

  for (size_t depth = 0; depth < GetMaxDepth(); ++depth)
  {
    auto [possible_games, en_croissants, castlings, ends_of_game] =
        CountPossibleGames(position, depth);

    const auto& [possible_games_answer, en_croissants_answer, castlings_answer,
                 ends_of_game_answer] = GetInfo(depth);

    EXPECT_EQ(position, GetPosition());
    EXPECT_EQ(position.GetHash(), GetPosition().GetHash());
    if (possible_games_answer)
      EXPECT_EQ(*possible_games, *possible_games_answer);
    if (en_croissants_answer) EXPECT_EQ(*en_croissants, *en_croissants_answer);
    if (castlings_answer) EXPECT_EQ(*castlings, *castlings_answer);
  }
}

INSTANTIATE_TEST_CASE_P(
    PerftTests, GenerateMovesTest,
    ::testing::Values(
        GenTestCase{
            R"(r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - )",
            {
                {1, 0, 0, 0},                // depth 0
                {48, 0, 2, 0},               // depth 1
                {2039, 1, 91, 0},            // depth 2
                {97862, 45, 3162, 0},        // depth 3
                {4085603, 1929, 128013, 1},  // depth 4
            }},
        GenTestCase{R"(8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -)",
                    {
                        {1, 0, 0, 0},            // depth 0
                        {14, 0, 0, 0},           // depth 1
                        {191, 0, 0, 0},          // depth 2
                        {2812, 2, 0, 0},         // depth 3
                        {43238, 123, 0, 0},      // depth 4
                        {674624, 1165, 0, 17},   // depth 5
                        {11030083, 33325, 0, 0}  // depth 6
                    }},
        GenTestCase{
            R"(r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1)",
            {
                {1, 0, 0, 0},           // depth 0
                {6, 0, 0, 0},           // depth 1
                {264, 0, 6, 0},         // depth 2
                {9467, 4, 0, 0},        // depth 3
                {422333, 0, 7795, 22},  // depth 4
                {15833292, 6512, 0, 5}  // depth 5
            }},
        GenTestCase{
            R"(r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1)",
            {
                {1, 0, 0, 0},           // depth 0
                {6, 0, 0, 0},           // depth 1
                {264, 0, 6, 0},         // depth 2
                {9467, 4, 0, 0},        // depth 3
                {422333, 0, 7795, 22},  // depth 4
                {15833292, 6512, 0, 5}  // depth 5
            }},
        GenTestCase{
            R"(r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1)",
            {
                {1, 0, 0, 0},           // depth 0
                {6, 0, 0, 0},           // depth 1
                {264, 0, 6, 0},         // depth 2
                {9467, 4, 0, 0},        // depth 3
                {422333, 0, 7795, 22},  // depth 4
                {15833292, 6512, 0, 5}  // depth 5
            }},
        GenTestCase{
            R"(rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8)",
            {
                {1},       // depth 0
                {44},      // depth 1
                {1486},    // depth 2
                {62379},   // depth 3
                {2103487}  // depth 4
            }}));

TEST(GenerateMoves, ShannonNumberCheck)
{
  auto start_position = PositionFactory{}();

  constexpr size_t max_plies = 6;

  static constexpr std::array<size_t, max_plies + 1> shannon_number = {
      1, 20, 400, 8902, 197281, 4865609, 119060324};
  static constexpr std::array<size_t, max_plies + 1> en_croissants_answer = {
      0, 0, 0, 0, 0, 258, 5248};
  static constexpr std::array<size_t, max_plies + 1> castlings_answer = {
      0, 0, 0, 0, 0, 0, 0};
  static constexpr std::array<size_t, max_plies + 1> ends_of_game_answer = {
      0, 0, 0, 0, 0, 8, 347};

  for (size_t depth = 0; depth <= max_plies; ++depth)
  {
    auto [possible_games, en_croissants, castlings, ends_of_game] =
        CountPossibleGames(start_position, depth);

    ASSERT_EQ(start_position, PositionFactory{}());
    EXPECT_EQ(start_position.GetHash(), PositionFactory{}().GetHash());
    ASSERT_EQ(possible_games, shannon_number[depth]);
    ASSERT_EQ(en_croissants, en_croissants_answer[depth]);
    ASSERT_EQ(castlings, castlings_answer[depth]);
    ASSERT_EQ(ends_of_game, ends_of_game_answer[depth]);
  }
}
}  // namespace MoveGeneratorTests

int main(int* argc, char** argv)
{
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  SimpleChessEngine::InitPawnAttacks();
  SimpleChessEngine::InitPSQT();

  RUN_ALL_TESTS();
}