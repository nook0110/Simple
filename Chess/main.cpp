#include "Attacks.cpp"
#include "Attacks.h"
#include "SimpleChessEngine.h"
#include "UciCommunicator.h"

int main()
{
  constexpr unsigned long long seed = 239;

  std::mt19937_64 gen(seed);
  std::uniform_int_distribution square_gen(0, 63);
  std::cout
      << "INSTANTIATE_TEST_CASE_P(RandomBoardBishop, BishopAttackMapTest,\n\
                  testing::Values(";
  const int test_cases = 25;
  for (int i = 0; i < test_cases; ++i)
  {
    Bitboard occupancy{gen() & gen()};
    BitIndex sq = square_gen(gen);
    occupancy.reset(sq);
    Bitboard ans = GenerateAttackMask<Piece::kRook>(sq, occupancy);
    // std::cout << DrawBitboard(occupancy) << std::endl;
    // std::cout << DrawBitboard(ans) << std::endl;
    std::cout << std::hex << "TestCaseWithoutAnswer{ Bitboard{0x"
              << (size_t)occupancy << "}, " << std::dec << sq << "}";
    if (i != test_cases - 1) std::cout << ",\n";
  }
  std::cout << "));";
  SimpleChessEngine::UciChessEngine uci;
  uci.Start();
}