#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

#include "Evaluation.h"
#include "Settings.h"

namespace SimpleChessEngine {
struct RootScore {
  std::optional<Eval> average;
  std::int64_t mean_squared = 0;

  void Update(Eval value) {
    const auto wide_value = static_cast<std::int64_t>(value);
    const auto squared = wide_value * std::abs(wide_value);
    mean_squared = average ? (squared + mean_squared) / 2 : squared;
    average = average ? static_cast<Eval>((wide_value + *average) / 2) : value;
  }
};

class Window {
  using Parameters = Settings::SearchParameters::AspirationWindowSettings;

 public:
  static constexpr Eval neg_inf = std::numeric_limits<Eval>::min() / 2;
  static constexpr Eval pos_inf = std::numeric_limits<Eval>::max() / 2;

  explicit Window(const RootScore& score = {}, std::size_t thread_index = 0) {
    if (!score.average) return;
    delta_ = std::min<double>(
        pos_inf, Parameters::kInitialDelta +
                     Parameters::kThreadDelta * (thread_index % 8) +
                     std::abs(score.mean_squared) /
                         Parameters::kMeanSquaredScoreDivisor);
    lower_bound_ = Clamp(static_cast<std::int64_t>(*score.average) - Margin());
    upper_bound_ = Clamp(static_cast<std::int64_t>(*score.average) + Margin());
  }

  Eval GetLowerBound() const { return lower_bound_; }
  Eval GetUpperBound() const { return upper_bound_; }

  void FailedLower(Eval best_value) {
    upper_bound_ = lower_bound_;
    lower_bound_ = Clamp(static_cast<std::int64_t>(best_value) - Margin());
    failed_high_count_ = 0;
    Grow();
  }

  void FailedUpper(Eval best_value) {
    lower_bound_ =
        std::max(Clamp(static_cast<std::int64_t>(upper_bound_) - Margin()),
                 lower_bound_);
    upper_bound_ = Clamp(static_cast<std::int64_t>(best_value) + Margin());
    ++failed_high_count_;
    Grow();
  }

  void ResetDepthReduction() { failed_high_count_ = 0; }

  Depth AdjustedDepth(Depth root_depth) const {
    return static_cast<Depth>(
        std::max(1, static_cast<int>(root_depth) - failed_high_count_));
  }

 private:
  static Eval Clamp(std::int64_t value) {
    return static_cast<Eval>(std::clamp<std::int64_t>(value, neg_inf, pos_inf));
  }

  Eval Margin() const { return static_cast<Eval>(std::round(delta_)); }
  void Grow() {
    delta_ =
        std::min<double>(pos_inf, delta_ + delta_ * Parameters::kDeltaGrowth);
  }

  double delta_ = 0;
  Eval lower_bound_ = neg_inf;
  Eval upper_bound_ = pos_inf;
  int failed_high_count_ = 0;
};
}
