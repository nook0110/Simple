#pragma once

#include <array>
#include <cassert>
#include <optional>

#include "Evaluation.h"
#include "Utility.h"

namespace SimpleChessEngine {
struct EvaluationTrend {
  bool improving = false;
  bool opponent_worsening = false;
};

class StaticEvalHistory {
 public:
  void Clear() { values_ = {}; }

  [[nodiscard]] EvaluationTrend Update(Depth ply, std::optional<Eval> eval) {
    assert(ply < values_.size());
    values_[ply] = eval;
    if (!eval) return {};

    std::optional<Eval> previous;
    if (ply >= 2) previous = values_[ply - 2];
    if (!previous && ply >= 4) previous = values_[ply - 4];

    return {previous && *eval > *previous,
            ply >= 1 && values_[ply - 1] && *eval > -*values_[ply - 1]};
  }

 private:
  std::array<std::optional<Eval>, kMaxSearchPly + 1> values_{};
};
}
