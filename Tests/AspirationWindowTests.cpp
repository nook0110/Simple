#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "AspirationWindow.h"
#include "MoveFactory.h"
#include "SimpleChessEngine.h"

using namespace SimpleChessEngine;

TEST(AspirationWindow, StartsWithFullWindowWithoutScores) {
  const Window window;
  EXPECT_EQ(window.GetLowerBound(), Window::neg_inf);
  EXPECT_EQ(window.GetUpperBound(), Window::pos_inf);
}

TEST(AspirationWindow, UsesSmoothedSignedRootScoresAndConvertedMargins) {
  RootScore score;
  score.Update(100);
  EXPECT_EQ(score.average, 100);
  EXPECT_EQ(score.mean_squared, 10000);
  score.Update(-50);
  EXPECT_EQ(score.average, 25);
  EXPECT_EQ(score.mean_squared, 3750);
  const Window window{score};
  EXPECT_EQ(window.GetLowerBound(), 22);
  EXPECT_EQ(window.GetUpperBound(), 28);
}

TEST(AspirationWindow, FailLowLowersAlphaAndSetsBetaToOldAlpha) {
  Window window{{100, 0}};
  window.FailedLower(90);
  EXPECT_EQ(window.GetLowerBound(), 88);
  EXPECT_EQ(window.GetUpperBound(), 98);
  window.FailedLower(80);
  EXPECT_EQ(window.GetLowerBound(), 77);
  EXPECT_EQ(window.GetUpperBound(), 88);
}

TEST(AspirationWindow, FailHighRaisesBetaAndReducesDepth) {
  Window window{{100, 0}};
  window.FailedUpper(110);
  EXPECT_EQ(window.GetLowerBound(), 100);
  EXPECT_EQ(window.GetUpperBound(), 112);
  EXPECT_EQ(window.AdjustedDepth(8), 7);
  window.FailedUpper(120);
  EXPECT_EQ(window.GetLowerBound(), 109);
  EXPECT_EQ(window.GetUpperBound(), 123);
  EXPECT_EQ(window.AdjustedDepth(8), 6);
  EXPECT_EQ(window.AdjustedDepth(1), 1);
  window.FailedLower(90);
  EXPECT_EQ(window.GetLowerBound(), 86);
  EXPECT_EQ(window.GetUpperBound(), 109);
  EXPECT_EQ(window.AdjustedDepth(8), 8);
}

TEST(AspirationWindow, ClampsInitialAndFailedBounds) {
  Window high{{Window::pos_inf - 1, 0}};
  EXPECT_EQ(high.GetUpperBound(), Window::pos_inf);
  high.FailedUpper(std::numeric_limits<Eval>::max());
  EXPECT_EQ(high.GetUpperBound(), Window::pos_inf);
  Window low{{Window::neg_inf + 1, 0}};
  EXPECT_EQ(low.GetLowerBound(), Window::neg_inf);
  Window failed_low{{Window::neg_inf + 10, 0}};
  failed_low.FailedLower(std::numeric_limits<Eval>::min());
  EXPECT_EQ(failed_low.GetLowerBound(), Window::neg_inf);
  EXPECT_LT(failed_low.GetLowerBound(), failed_low.GetUpperBound());
  RootScore score;
  score.Update(Window::pos_inf);
  Window wide{score};
  EXPECT_EQ(wide.GetLowerBound(), 0);
  EXPECT_EQ(wide.GetUpperBound(), Window::pos_inf);
}

TEST(AspirationWindow, WidensUntilMateAndTablebaseScoresFit) {
  for (const Eval value :
       {Score::MatedIn(1), Score::MateIn(1), Score::kTbWin, -Score::kTbWin}) {
    Window window{{0, 0}};
    int attempts = 0;
    while (
        (value <= window.GetLowerBound() || value >= window.GetUpperBound()) &&
        attempts++ < 100) {
      if (value <= window.GetLowerBound()) {
        window.FailedLower(window.GetLowerBound());
      } else {
        window.FailedUpper(window.GetUpperBound());
      }
    }
    EXPECT_LT(attempts, 100);
    EXPECT_GT(value, window.GetLowerBound());
    EXPECT_LT(value, window.GetUpperBound());
  }
}

namespace {
class RetryOutput : public std::stringbuf {
 public:
  enum class Failure { Low, High, Verification };
  explicit RetryOutput(Failure failure) : failure_(failure) {}

  std::function<void()> on_retry;
  std::string completed_pv;
  std::string completed_score;
  std::vector<std::string> pvs;
  std::vector<std::string> scores;
  Eval failed_beta = 0;
  bool stopped = false;

 protected:
  std::streamsize xsputn(const char* text, std::streamsize count) override {
    const auto result = std::stringbuf::xsputn(text, count);
    pending_.append(text, static_cast<std::size_t>(count));
    for (auto end = pending_.find('\n'); end != std::string::npos;
         end = pending_.find('\n')) {
      ReadLine(pending_.substr(0, end));
      pending_.erase(0, end + 1);
    }
    return result;
  }

 private:
  void ReadLine(const std::string& line) {
    if (line.starts_with("info score ")) scores.push_back(line);
    if (line.starts_with("info depth ")) {
      if (line.find(" pv") != std::string::npos) {
        pvs.push_back(line);
      } else {
        attempts_ = 0;
      }
    }
    if (!line.starts_with("info window ")) return;
    std::istringstream input(line.substr(12));
    Eval lower, upper;
    input >> lower >> upper;
    ++attempts_;
    const bool failed_high = upper > previous_upper_;
    if (attempts_ > 1 && failed_high) last_high_score_ = previous_upper_;
    const bool matches =
        failure_ == Failure::Low ? upper < previous_upper_
        : failure_ == Failure::High
            ? failed_high
            : lower == previous_lower_ && upper == previous_upper_;
    const bool eligible =
        failure_ == Failure::Verification ? attempts_ > 2 : attempts_ == 2;
    if (!stopped && eligible && matches && !pvs.empty()) {
      completed_pv = pvs.back();
      completed_score = scores.back();
      failed_beta = failure_ == Failure::Verification ? last_high_score_
                                                      : previous_upper_;
      stopped = true;
      on_retry();
    }
    previous_lower_ = lower;
    previous_upper_ = upper;
  }

  Failure failure_;
  std::string pending_;
  int attempts_ = 0;
  Eval previous_lower_ = 0;
  Eval previous_upper_ = 0;
  Eval last_high_score_ = 0;
};
}

TEST(AspirationSearch, StoppingAfterFailLowPreservesCompletedMoveScoreAndPv) {
  RetryOutput buffer{RetryOutput::Failure::Low};
  std::ostream output(&buffer);
  ChessEngine engine{PositionFactory{}(), output};
  auto stop = std::make_shared<std::atomic_bool>(false);
  DepthCondition condition{8, stop};
  Depth completed_depth = 0;
  Move trusted_move = Move::None();
  buffer.on_retry = [&] {
    trusted_move = engine.GetCurrentBestMove();
    completed_depth = condition.cur_depth;
    stop->store(true, std::memory_order_relaxed);
  };
  engine.ComputeBestMove(condition);
  ASSERT_TRUE(buffer.stopped) << buffer.str();
  EXPECT_GT(completed_depth, 0);
  EXPECT_EQ(engine.GetCurrentBestMove(), trusted_move);
  ASSERT_FALSE(buffer.pvs.empty());
  EXPECT_EQ(buffer.pvs.back(), buffer.completed_pv);
  EXPECT_EQ(buffer.scores.back(), buffer.completed_score);
  EXPECT_EQ(condition.cur_depth, completed_depth);
  std::ostringstream best_move;
  best_move << "bestmove " << trusted_move;
  EXPECT_NE(buffer.str().find(best_move.str()), std::string::npos);
}

TEST(AspirationSearch, StoppingAfterFailHighKeepsProvenRootMove) {
  RetryOutput buffer{RetryOutput::Failure::High};
  std::ostream output(&buffer);
  ChessEngine engine{PositionFactory{}(), output};
  auto stop = std::make_shared<std::atomic_bool>(false);
  DepthCondition condition{8, stop};
  Depth completed_depth = 0;
  Move trusted_move = Move::None();
  buffer.on_retry = [&] {
    trusted_move = engine.GetCurrentBestMove();
    completed_depth = condition.cur_depth;
    stop->store(true, std::memory_order_relaxed);
  };
  engine.ComputeBestMove(condition);
  ASSERT_TRUE(buffer.stopped) << buffer.str();
  EXPECT_GT(completed_depth, 0);
  EXPECT_EQ(engine.GetCurrentBestMove(), trusted_move);
  EXPECT_EQ(buffer.scores.back(),
            "info score cp " + std::to_string(buffer.failed_beta));
  std::ostringstream pv;
  pv << " pv " << trusted_move;
  EXPECT_NE(buffer.pvs.back().find(pv.str()), std::string::npos);
  EXPECT_EQ(condition.cur_depth, completed_depth);
}

TEST(AspirationSearch, ResetsRootMoveScoresBetweenSearches) {
  Searcher searcher{PositionFactory{}()};
  searcher.InitStartOfSearch();
  DepthCondition condition{1};
  const auto result = searcher.Search<NodeType::kPV>(
      condition, 1, 1, Window::neg_inf, Window::pos_inf);
  ASSERT_TRUE(result);
  const auto move = searcher.GetCurrentBestMove();
  const auto score = searcher.GetRootScore(move);
  EXPECT_EQ(score.average, result);
  EXPECT_EQ(score.mean_squared,
            static_cast<std::int64_t>(*result) * std::abs(*result));
  EXPECT_GT(searcher.GetRootEffort(move), 0u);
  searcher.InitStartOfSearch();
  EXPECT_FALSE(searcher.GetRootScore(move).average);
  EXPECT_EQ(searcher.GetRootEffort(move), 0u);
}

TEST(AspirationWindow, RestoresFullDepthWithoutChangingBounds) {
  Window window{{100, 0}};
  window.FailedUpper(110);
  const Eval lower = window.GetLowerBound();
  const Eval upper = window.GetUpperBound();
  ASSERT_EQ(window.AdjustedDepth(8), 7);
  window.ResetDepthReduction();
  EXPECT_EQ(window.AdjustedDepth(8), 8);
  EXPECT_EQ(window.GetLowerBound(), lower);
  EXPECT_EQ(window.GetUpperBound(), upper);
}

TEST(AspirationWindow, ThreadIndexOffsetsWrapAtEight) {
  const Window main{{0, 0}, 0};
  const Window helper{{0, 0}, 7};
  const Window wrapped{{0, 0}, 15};
  EXPECT_EQ(main.GetUpperBound(), 2);
  EXPECT_EQ(helper.GetLowerBound(), -5);
  EXPECT_EQ(helper.GetUpperBound(), 5);
  EXPECT_EQ(wrapped.GetLowerBound(), helper.GetLowerBound());
  EXPECT_EQ(wrapped.GetUpperBound(), helper.GetUpperBound());
}

TEST(AspirationSearch, FindsQueenSacrificeMateAtCompletedDepthEight) {
  const auto position =
      PositionFactory{}("1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - - 0 1");
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  std::ostringstream output;
  ChessEngine engine{position, output, table};
  DepthCondition condition{8};
  engine.ComputeBestMove(condition);
  EXPECT_EQ(engine.GetCurrentBestMove(), MoveFactory{}(position, "d6d1"));
  const auto root = table->GetNode(position);
  EXPECT_EQ(root.bound, Bound::kExact);
  EXPECT_GE(root.depth, 8);
  EXPECT_EQ(root.score, Score::MateIn(5));
  EXPECT_NE(output.str().find("info score mate 3"), std::string::npos);
}

TEST(AspirationSearch, PrintsCompletedDepthWhenPvIsShorter) {
  auto position = PositionFactory{}();
  MoveGenerator::Moves moves{MoveFactory{}(position, "e2e4")};
  position.DoMove(moves.front());
  moves.push_back(MoveFactory{}(position, "e7e5"));
  std::ostringstream output;
  output << PrincipalVariationInfo{18, moves};
  EXPECT_EQ(output.str(), "info depth 18 pv e2e4 e7e5\n");
}

TEST(AspirationSearch, UsesPerEngineThreadIndexForWindows) {
  const auto position = PositionFactory{}();
  std::ostringstream output;
  ChessEngine engine{position,
                     output,
                     std::make_shared<Searcher::SearcherTranspositionTable>(),
                     true,
                     1,
                     {},
                     7};
  DepthCondition condition{2};
  engine.ComputeBestMove(condition);
  std::istringstream lines(output.str());
  std::string line;
  Eval first_score = 0;
  int windows = 0;
  bool checked = false;
  while (std::getline(lines, line)) {
    if (line.starts_with("info score cp ") && windows == 1) {
      std::istringstream(line.substr(14)) >> first_score;
    }
    if (line.starts_with("info window ") && ++windows == 2) {
      RootScore score;
      score.Update(first_score);
      const Window expected{score, 7};
      Eval lower = 0;
      Eval upper = 0;
      std::istringstream(line.substr(12)) >> lower >> upper;
      EXPECT_EQ(lower, expected.GetLowerBound());
      EXPECT_EQ(upper, expected.GetUpperBound());
      checked = true;
      break;
    }
  }
  EXPECT_TRUE(checked);
}

TEST(AspirationWindow, UsesStockfish19SignedSquareDivisor) {
  RootScore score;
  score.Update(1000);
  const Window window{score};
  EXPECT_EQ(window.GetLowerBound(), 749);
  EXPECT_EQ(window.GetUpperBound(), 1251);
}

TEST(AspirationWindow, GrowsDeltaByStockfish19Ratio) {
  Window window{{0, 0}};
  for (int failure = 0; failure < 5; ++failure) {
    window.FailedUpper(window.GetUpperBound());
  }
  EXPECT_EQ(window.GetLowerBound(), 9);
  EXPECT_EQ(window.GetUpperBound(), 23);
}

TEST(AspirationSearch,
     StoppingDuringFullDepthVerificationKeepsTrustedFailHigh) {
  const auto position =
      PositionFactory{}("1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - - 0 1");
  RetryOutput buffer{RetryOutput::Failure::Verification};
  std::ostream output(&buffer);
  ChessEngine engine{position, output};
  auto stop = std::make_shared<std::atomic_bool>(false);
  DepthCondition condition{8, stop};
  Depth completed_depth = 0;
  Move trusted_move = Move::None();
  buffer.on_retry = [&] {
    completed_depth = condition.cur_depth;
    trusted_move = engine.GetCurrentBestMove();
    stop->store(true, std::memory_order_relaxed);
  };
  engine.ComputeBestMove(condition);
  ASSERT_TRUE(buffer.stopped) << buffer.str();
  EXPECT_EQ(condition.cur_depth, completed_depth);
  EXPECT_EQ(engine.GetCurrentBestMove(), trusted_move);
  EXPECT_EQ(buffer.scores.back(),
            "info score cp " + std::to_string(buffer.failed_beta));
  std::ostringstream pv;
  pv << " pv " << trusted_move;
  EXPECT_NE(buffer.pvs.back().find(pv.str()), std::string::npos);
}
