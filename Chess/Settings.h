#pragma once
#include <ostream>

#include "Evaluation.h"
#include "Utility.h"

namespace SimpleChessEngine {
struct Settings {
  struct EvaluationParameters {
    inline static TaperedEval doubled_pawn{{-15, -15}};
    inline static TaperedEval isolated_pawn{{-10, -10}};
    inline static std::array<TaperedEval, 8> passed_pawn = [] {
      std::array<TaperedEval, 8> values{};
      values[3] = TaperedEval{{5, 18}};
      values[4] = TaperedEval{{30, 40}};
      values[5] = TaperedEval{{25, 70}};
      values[6] = TaperedEval{{90, 120}};
      return values;
    }();
    inline static std::array<std::array<TaperedEval, 8>, 8>
        pawn_psqt_adjustment = [] {
          std::array<std::array<TaperedEval, 8>, 8> values{};
          values[5] = {
              TaperedEval{{6, -59}},   TaperedEval{{-2, -60}},
              TaperedEval{{-11, -50}}, TaperedEval{{-11, -37}},
              TaperedEval{{-40, -26}}, TaperedEval{{-31, -23}},
              TaperedEval{{-10, -47}}, TaperedEval{{20, -49}},
          };
          values[6] = {
              TaperedEval{{-83, -128}}, TaperedEval{{-109, -118}},
              TaperedEval{{-41, -108}}, TaperedEval{{-70, -89}},
              TaperedEval{{-48, -102}}, TaperedEval{{-101, -87}},
              TaperedEval{{-19, -115}}, TaperedEval{{21, -132}},
          };
          return values;
        }();
    inline static std::array<TaperedEval, kPieceTypes> material_value = [] {
      auto values = kPieceValues;
      values[static_cast<size_t>(Piece::kKnight)]
          .eval[static_cast<size_t>(GamePhase::kEndGame)] = 275;
      values[static_cast<size_t>(Piece::kBishop)]
          .eval[static_cast<size_t>(GamePhase::kMiddleGame)] = 375;
      return values;
    }();
    inline static bool pawns_enabled = true;
    inline static bool material_enabled = true;
    inline static size_t revision = 0;

    static void SetDoubledPawn(GamePhase phase, Eval value) {
      doubled_pawn.eval[static_cast<size_t>(phase)] = value;
      RefreshPawnsEnabled();
    }

    static void SetIsolatedPawn(GamePhase phase, Eval value) {
      isolated_pawn.eval[static_cast<size_t>(phase)] = value;
      RefreshPawnsEnabled();
    }

    static void SetPassedPawn(size_t relative_rank, GamePhase phase,
                              Eval value) {
      passed_pawn[relative_rank].eval[static_cast<size_t>(phase)] = value;
      RefreshPawnsEnabled();
    }

    static void SetPawnPsqtAdjustment(size_t relative_rank, size_t file,
                                      GamePhase phase, Eval value) {
      pawn_psqt_adjustment[relative_rank][file]
          .eval[static_cast<size_t>(phase)] = value;
      RefreshPawnsEnabled();
    }

    static void SetMaterialValue(Piece piece, GamePhase phase, Eval value) {
      material_value[static_cast<size_t>(piece)]
          .eval[static_cast<size_t>(phase)] = value;
      material_enabled = material_value != kPieceValues;
    }

   private:
    static void RefreshPawnsEnabled() {
      ++revision;
      pawns_enabled = doubled_pawn != TaperedEval{} ||
                      isolated_pawn != TaperedEval{};
      for (const auto& bonus : passed_pawn) {
        pawns_enabled |= bonus != TaperedEval{};
      }
      for (const auto& rank : pawn_psqt_adjustment) {
        for (const auto& adjustment : rank) {
          pawns_enabled |= adjustment != TaperedEval{};
        }
      }
    }
  };

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
