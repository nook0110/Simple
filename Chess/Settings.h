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
    struct IIRSettings {
      static constexpr Depth kBaseLimit = 2;
      static constexpr Depth kCutNodePenalty = 0;
      static constexpr Depth kReduction = 1;
    };
    struct LMRSettings {
      static constexpr bool kEnabled = true;
      static constexpr size_t kDepthLimit = 4;
      static constexpr int kUnderCheckReductionPenalty = 1;
      static constexpr int kDoingCheckReductionPenalty = 1;
    };
    static constexpr RFPSettings kRFPSettings = {};
    static constexpr NMPSettings kNMPSettings = {};
    static constexpr IIRSettings kIIRSettings = {};
    static constexpr LMRSettings kLMRSettings = {};
  };
  struct SearchParameters {
    struct AspirationWindowSettings {
      enum class Strategy { Linear, Exponential };
      static constexpr bool kEnabled = false;
      static constexpr Eval kPawnValue =
          kPieceValues[static_cast<size_t>(Piece::kPawn)]
              .eval[static_cast<size_t>(GamePhase::kMiddleGame)];
      static constexpr Eval kDelta = kPawnValue * 3 / 4;
      static constexpr Strategy kStrategy = Strategy::Linear;
    };
    static constexpr AspirationWindowSettings kAspirationWindowSettings = {};
  };
  static constexpr PruneParameters kPruneParameters = {};
  static constexpr SearchParameters kSearchParameters = {};
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
std::ostream& operator<<(std::ostream& out,
                         Settings::SearchParameters parameters);
std::ostream& operator<<(std::ostream& out,
                         Settings::PruneParameters parameters);
}  // namespace SimpleChessEngine
