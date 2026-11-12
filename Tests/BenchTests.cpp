#include <gtest/gtest.h>

#include <cstdlib>
#include <sstream>
#include <string>

#include "Bench.h"
#include "Nnue/Nnue.h"

namespace SimpleChessEngine {
TEST(SearchBench, RequiresLoadedNetwork) {
  Nnue::Unload();
  std::ostringstream output;
  RunSearchBench(output, 1);
  EXPECT_EQ(output.str(), "info string Bench requires a loaded NNUE network\n");
}

TEST(SearchBench, RepeatedRunsStartWithFreshSearchState) {
  const auto* network = std::getenv("SCE_NNUE");
  if (network == nullptr) GTEST_SKIP() << "SCE_NNUE is not set";
  ASSERT_TRUE(Nnue::Load(network));
  std::ostringstream first;
  std::ostringstream second;
  RunSearchBench(first, 3);
  RunSearchBench(second, 3);
  std::istringstream first_lines{first.str()};
  std::istringstream second_lines{second.str()};
  std::string first_signature;
  std::string second_signature;
  std::getline(first_lines, first_signature);
  std::getline(second_lines, second_signature);
  EXPECT_TRUE(first_signature.starts_with("Nodes searched: "));
  EXPECT_EQ(first_signature, second_signature);
}
}
