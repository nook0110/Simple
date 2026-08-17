#pragma once
#include <ostream>

#include "Evaluation.h"
#include "Utility.h"

namespace SimpleChessEngine {
struct Settings {
  struct PruneParameters {
    struct RFPSettings {
      static constexpr bool kEnabled = true;
      inline static Depth kDepthLimit = 5;
      inline static Eval kThreshold = 100;
    };
    struct NMPSettings {
      static constexpr bool kEnabled = true;
      inline static size_t kNullMoveReduction = 3;
    };
    struct IIRSettings {
      inline static Depth kBaseLimit = 2;
      inline static Depth kCutNodePenalty = 1;
      inline static Depth kReduction = 1;
    };
    struct LMRSettings {
      static constexpr bool kEnabled = true;
      inline static size_t kDepthLimit = 3;
      inline static int kUnderCheckReductionPenalty = 1;
      inline static int kDoingCheckReductionPenalty = 2;
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
