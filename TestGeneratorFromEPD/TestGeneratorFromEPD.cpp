#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

class TestGenerator
{
 public:
  explicit TestGenerator(std::string compute_best_start,
                         std::string compute_best_end = ");")
      : compute_best_start_(std::move(compute_best_start)),
        compute_best_end_(std::move(compute_best_end))
  {}

  [[nodiscard]] std::string operator()(const std::string& epd) const
  {
    static const std::string best_move_word = "bm";
    const auto end_of_fen = epd.find(best_move_word);
    assert(end_of_fen < epd.size());  // only epd with best move
    const auto fen = epd.substr(0, end_of_fen);
    const auto position = GeneratePosition(fen);

    static const std::string best_move_end = ";";
    const auto end_of_move = epd.find(best_move_end);

    const auto move_start = end_of_fen + best_move_word.size();
    const auto move =
        GenerateMove(epd.substr(move_start + 1, end_of_move - move_start));

    const auto id = GetId(epd);

    auto full_function = GenerateFunctionSyntax(id);
    full_function += GenerateBody(position);
    full_function += GenerateAssert(move);

    static const std::string end_of_function = "}";
    return full_function += end_of_function;
  }

 private:
  static [[nodiscard]] std::string GenerateFunctionSyntax(const std::string& id)
  {
    static const std::string test_start = "TEST(PositionTest, ";
    static const std::string test_end = ")" + kNewLine + "{" + kNewLine;

    return test_start + id + test_end;
  }

  static [[nodiscard]] std::string GeneratePosition(const std::string& fen)
  {
    static const std::string generator_start = R"(PositionFactory{}(")";
    static const std::string generator_end = R"("))";

    return generator_start + fen + generator_end;
  }

  [[nodiscard]] std::string GenerateBody(const std::string& position) const
  {
    static const std::string body_start = "const auto move = ";

    return body_start + compute_best_start_ + position + compute_best_end_ +
           kNewLine;
  }

  static [[nodiscard]] std::string GenerateMove(const std::string& move)
  {
    static const std::string move_start = R"(MoveFactory{}(")";
    static const std::string move_end = R"("))";

    return move_start + move + move_end;
  }

  static [[nodiscard]] std::string GenerateAssert(const std::string& move)
  {
    static const std::string assert_start = "ASSERT_EQ(move, ";
    static const std::string assert_end = ");";

    return assert_start + move + assert_end + kNewLine;
  }

  static [[nodiscard]] std::string GetId(const std::string& epd)
  {
    const auto start_of_id = epd.find("id");

    auto id = epd.substr(start_of_id);

    for (const std::vector prohibited = {'"', ';', ' ', '.'};
         auto c : prohibited)
    {
      std::erase(id, c);
    }
    return id;
  }

  const std::string compute_best_start_;
  const std::string compute_best_end_;
  static const std::string kNewLine;
};

const std::string TestGenerator::kNewLine = "\n";

int main()
{
  const TestGenerator time_generator("ComputeBestMoveByTime(");
  std::cout << time_generator(
      R"(1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - - bm Qd1+; id "BK.01";)");
}