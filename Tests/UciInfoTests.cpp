#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "SimpleChessEngine.h"

namespace SimpleChessEngine::UciInfoTests {
TEST(PrincipalVariationInfo, ReportsIterationDepthForShortPV) {
  std::ostringstream output;
  output << PrincipalVariationInfo{14, {Move{12, 28}, Move{52, 36}}};
  EXPECT_EQ(output.str(), "info depth 14 pv e2e4 e7e5\n");
}

TEST(PrincipalVariationInfo, ReportsIterationDepthForEmptyPV) {
  std::ostringstream output;
  output << PrincipalVariationInfo{14, {}};
  EXPECT_EQ(output.str(), "info depth 14 pv\n");
}

std::vector<int> PVDepths(const std::string& output) {
  std::istringstream input(output);
  std::vector<int> depths;
  for (std::string line; std::getline(input, line);) {
    if (!line.starts_with("info depth ") ||
        line.find(" pv") == std::string::npos) {
      continue;
    }
    std::istringstream fields(line);
    std::string token;
    int depth = 0;
    fields >> token >> token >> depth;
    depths.push_back(depth);
  }
  return depths;
}

TEST(UciDepth, FinalReportUsesCompletedIteration) {
  std::ostringstream output;
  ChessEngine engine{PositionFactory{}(), output};
  DepthCondition condition{3};
  engine.ComputeBestMove(condition);
  EXPECT_EQ(PVDepths(output.str()), (std::vector<int>{1, 2, 3, 3}));
}

struct StopAtDepthBuffer : std::stringbuf {
  std::streamsize xsputn(const char* data, std::streamsize size) override {
    if (std::string_view{data, static_cast<std::size_t>(size)} ==
        "info depth 3\n") {
      stop->store(true, std::memory_order_relaxed);
    }
    return std::stringbuf::xsputn(data, size);
  }

  StopSignal stop = std::make_shared<std::atomic_bool>(false);
};

TEST(UciDepth, InterruptedIterationKeepsCompletedDepth) {
  StopAtDepthBuffer buffer;
  std::ostream output{&buffer};
  ChessEngine engine{PositionFactory{}(), output};
  DepthCondition condition{3, buffer.stop};
  engine.ComputeBestMove(condition);
  EXPECT_NE(buffer.str().find("info depth 3\n"), std::string::npos);
  EXPECT_EQ(PVDepths(buffer.str()), (std::vector<int>{1, 2, 2}));
}
}
