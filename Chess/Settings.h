#pragma once
#include <ostream>

#include "Evaluation.h"
#include "Utility.h"

namespace SimpleChessEngine {
struct Settings {
  struct PruneParameters {
    struct RFPSettings {
      static constexpr bool kEnabled = true;
      static constexpr Depth kDepthLimit = 5;
      static constexpr Eval kThreshold = 75;
    };
    struct NMPSettings {
      static constexpr bool kEnabled = true;
      static constexpr size_t kNullMoveReduction = 3;
    };
    static constexpr RFPSettings kRFPSettings;
    static constexpr NMPSettings kNMPSettings;
  };
  struct SearchParameters {
    struct AspirationWindowSettings {
      enum class Strategy { Linear, Exponential };
      static constexpr bool kEnabled = false;
      static constexpr Eval kDelta = 10;
      static constexpr Strategy kStrategy = Strategy::Linear;
    };
    static constexpr AspirationWindowSettings kAspirationWindowSettings;
  };
  static constexpr PruneParameters kPruneParameters;
  static constexpr SearchParameters kSearchParameters;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
std::ostream& operator<<(std::ostream& out,
                         Settings::SearchParameters parameters);
}  // namespace SimpleChessEngine
