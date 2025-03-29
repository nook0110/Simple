
#include "Chess/MoveGenerator.h"
#include "Chess/PositionFactory.h"
#include <gtest/gtest.h>

using namespace SimpleChessEngine;

namespace MoveGeneratorTests {
struct GameInfo {
  std::optional<size_t> possible_games{};

  std::optional<size_t> en_croissants{};
  std::optional<size_t> castlings{};

  std::optional<size_t> ends_of_game{};

  GameInfo &operator+=(const GameInfo &other) {
    possible_games.value() += other.possible_games.value();
    en_croissants.value() += other.en_croissants.value();
    castlings.value() += other.castlings.value();
    ends_of_game.value() += other.ends_of_game.value();
    return *this;
  }
};

[[nodiscard]] GameInfo CountPossibleGames(Position &position,
                                          const Depth depth) {
  if (depth == 0)
    return {1, 0, 0, 0};

  GameInfo answer{0, 0, 0, 0};

  static auto move_generator = MoveGenerator{};

  const auto moves =
      move_generator.GenerateMoves<MoveGenerator::Type::kDefault>(position);

  if (depth == 1) {
    for (const auto &move : moves) {
      if (std::get_if<EnCroissant>(&move)) {
        answer.en_croissants.value()++;
      }
      if (std::get_if<Castling>(&move)) {
        answer.castlings.value()++;
      }
    }

    if (moves.empty())
      answer.ends_of_game.value() += 1;

    answer.possible_games = moves.size();
    return answer;
  }

  for (const auto &move : moves) {
    const auto irreversible_data = position.GetIrreversibleData();
    position.DoMove(move);
    answer += CountPossibleGames(position, depth - 1);
    position.UndoMove(move, irreversible_data);
  }

  return answer;
}

struct GenTestCase {
  std::string fen;

  std::vector<GameInfo> infos;
};

class GenerateMovesTest : public testing::TestWithParam<GenTestCase> {
protected:
  void SetUp() override {
    const auto &[fen, infos] = GetParam();

    GeneratePosition();

    infos_ = infos;
  }

  [[nodiscard]] const Position &GetPosition() const { return position_; }

  [[nodiscard]] const GameInfo &GetInfo(const Depth depth) const {
    return infos_[depth];
  }

  [[nodiscard]] Depth GetMaxDepth() const { return infos_.size(); }

private:
  void GeneratePosition() { position_ = PositionFactory{}(GetParam().fen); }

  std::vector<GameInfo> infos_;
  Position position_;
};

TEST_P(GenerateMovesTest, Perft) {
  auto position = GetPosition();

  for (Depth depth = 0; depth < GetMaxDepth(); ++depth) {
    auto [possible_games, en_croissants, castlings, ends_of_game] =
        CountPossibleGames(position, depth);

    const auto &[possible_games_answer, en_croissants_answer, castlings_answer,
                 ends_of_game_answer] = GetInfo(depth);

    EXPECT_EQ(position, GetPosition());
    EXPECT_EQ(position.GetHash(), GetPosition().GetHash());
    if (possible_games_answer)
      EXPECT_EQ(*possible_games, *possible_games_answer);
    if (en_croissants_answer)
      EXPECT_EQ(*en_croissants, *en_croissants_answer);
    if (castlings_answer)
      EXPECT_EQ(*castlings, *castlings_answer);
  }
}

INSTANTIATE_TEST_CASE_P(
    PerftTests, GenerateMovesTest,
    ::testing::Values(
        GenTestCase{
            R"(r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1)",
            {
                {1, 0, 0, 0},               // depth 0
                {48, 0, 2, 0},              // depth 1
                {2039, 1, 91, 0},           // depth 2
                {97862, 45, 3162, 0},       // depth 3
                {4085603, 1929, 128013, 1}, // depth 4
            }},
        GenTestCase{R"(8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -)",
                    {
                        {1, 0, 0, 0},           // depth 0
                        {14, 0, 0, 0},          // depth 1
                        {191, 0, 0, 0},         // depth 2
                        {2812, 2, 0, 0},        // depth 3
                        {43238, 123, 0, 0},     // depth 4
                        {674624, 1165, 0, 17},  // depth 5
                        {11030083, 33325, 0, 0} // depth 6
                    }},
        GenTestCase{
            R"(r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1)",
            {
                {1, 0, 0, 0},          // depth 0
                {6, 0, 0, 0},          // depth 1
                {264, 0, 6, 0},        // depth 2
                {9467, 4, 0, 0},       // depth 3
                {422333, 0, 7795, 22}, // depth 4
                {15833292, 6512, 0, 5} // depth 5
            }},
        GenTestCase{
            R"(r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1)",
            {
                {1, 0, 0, 0},          // depth 0
                {6, 0, 0, 0},          // depth 1
                {264, 0, 6, 0},        // depth 2
                {9467, 4, 0, 0},       // depth 3
                {422333, 0, 7795, 22}, // depth 4
                {15833292, 6512, 0, 5} // depth 5
            }},
        GenTestCase{
            R"(r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1)",
            {
                {1, 0, 0, 0},          // depth 0
                {6, 0, 0, 0},          // depth 1
                {264, 0, 6, 0},        // depth 2
                {9467, 4, 0, 0},       // depth 3
                {422333, 0, 7795, 22}, // depth 4
                {15833292, 6512, 0, 5} // depth 5
            }},
        GenTestCase{
            R"(rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8)",
            {
                {1},      // depth 0
                {44},     // depth 1
                {1486},   // depth 2
                {62379},  // depth 3
                {2103487} // depth 4
            }}));

TEST(GenerateMoves, ShannonNumberCheck) {
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

  for (Depth depth = 0; depth <= max_plies - 1; ++depth) {
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
} // namespace MoveGeneratorTests
