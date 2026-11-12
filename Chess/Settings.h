#pragma once
#include <cstdint>
#include <ostream>
#include <type_traits>

#include "Evaluation.h"
#include "Score.h"
#include "Utility.h"

namespace SimpleChessEngine {
struct Settings {
  struct EvaluationParameters {
    inline static ByPiece<TaperedEval> mobility = [] {
      ByPiece<TaperedEval> values{};
      values[Piece::kKnight] = TaperedEval{{4, 2}};
      values[Piece::kBishop] = TaperedEval{{7, 3}};
      values[Piece::kRook] = TaperedEval{{2, 4}};
      values[Piece::kQueen] = TaperedEval{{2, 2}};
      return values;
    }();
    inline static ByPiece<std::array<std::array<TaperedEval, 4>, 8>>
        psqt_adjustment{};
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
    inline static ByPiece<TaperedEval> material_value = [] {
      auto values = kPieceValues;
      values[Piece::kKnight]
          .eval[static_cast<size_t>(GamePhase::kEndGame)] = 275;
      values[Piece::kBishop]
          .eval[static_cast<size_t>(GamePhase::kMiddleGame)] = 375;
      return values;
    }();
    inline static Eval king_shield_near = 12;
    inline static Eval king_shield_far = 10;
    inline static Eval king_semi_open_file = -14;
    inline static Eval king_open_file = -14;
    inline static Eval king_pawn_storm_near = -12;
    inline static Eval king_pawn_storm_far = -8;
    inline static ByPiece<Eval> king_attack = [] {
      ByPiece<Eval> values{};
      values[Piece::kKnight] = 8;
      values[Piece::kBishop] = 10;
      values[Piece::kRook] = 18;
      values[Piece::kQueen] = 20;
      return values;
    }();
    inline static bool pawns_enabled = true;
    inline static bool material_enabled = true;
    inline static bool mobility_enabled = true;
    inline static bool psqt_adjustment_enabled = false;
    inline static bool king_pawns_enabled = true;
    inline static bool king_attacks_enabled = true;
    inline static Eval nnue_scale = 0;
    inline static bool king_safety_enabled = true;
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
      material_value[piece]
          .eval[static_cast<size_t>(phase)] = value;
      material_enabled = material_value != kPieceValues;
    }

    static void SetMobility(Piece piece, GamePhase phase, Eval value) {
      mobility[piece].eval[static_cast<size_t>(phase)] =
          value;
      mobility_enabled = false;
      for (const auto& weight : mobility) {
        mobility_enabled |= weight != TaperedEval{};
      }
    }

    static void SetPsqtAdjustment(Piece piece, size_t relative_rank,
                                  size_t mirrored_file, GamePhase phase,
                                  Eval value) {
      psqt_adjustment[piece][relative_rank][mirrored_file]
          .eval[static_cast<size_t>(phase)] = value;
      psqt_adjustment_enabled = false;
      for (const auto& piece_values : psqt_adjustment) {
        for (const auto& rank : piece_values) {
          for (const auto& square : rank) {
            psqt_adjustment_enabled |= square != TaperedEval{};
          }
        }
      }
    }

    static void SetKingShieldNear(Eval value) {
      king_shield_near = value;
      RefreshKingSafetyEnabled();
    }

    static void SetKingShieldFar(Eval value) {
      king_shield_far = value;
      RefreshKingSafetyEnabled();
    }

    static void SetKingSemiOpenFile(Eval value) {
      king_semi_open_file = value;
      RefreshKingSafetyEnabled();
    }

    static void SetKingOpenFile(Eval value) {
      king_open_file = value;
      RefreshKingSafetyEnabled();
    }

    static void SetKingPawnStormNear(Eval value) {
      king_pawn_storm_near = value;
      RefreshKingSafetyEnabled();
    }

    static void SetKingPawnStormFar(Eval value) {
      king_pawn_storm_far = value;
      RefreshKingSafetyEnabled();
    }

    template <Piece piece>
    static void SetKingAttack(Eval value) {
      static_assert(piece == Piece::kKnight || piece == Piece::kBishop ||
                    piece == Piece::kRook || piece == Piece::kQueen);
      king_attack[piece] = value;
      RefreshKingSafetyEnabled();
    }

   private:
    static void RefreshKingSafetyEnabled() {
      ++revision;
      king_pawns_enabled = king_shield_near != 0 || king_shield_far != 0 ||
                           king_semi_open_file != 0 || king_open_file != 0 ||
                           king_pawn_storm_near != 0 ||
                           king_pawn_storm_far != 0;
      king_attacks_enabled = false;
      for (const auto weight : king_attack) {
        king_attacks_enabled |= weight != 0;
      }
      king_safety_enabled = king_pawns_enabled || king_attacks_enabled;
    }

    static void RefreshPawnsEnabled() {
      ++revision;
      pawns_enabled =
          doubled_pawn != TaperedEval{} || isolated_pawn != TaperedEval{};
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
      static constexpr Eval kNoTtCutNodeDiscount = 23;
      static constexpr int kImprovingDepthDeduction = 2;
      static constexpr int kOpponentWorseningDivisor = 3;

      Depth depth_limit = 5;
      Eval threshold = 100;

      [[nodiscard]] constexpr Eval Margin(Depth depth, bool no_tt_cut_node,
                                          bool improving,
                                          bool opponent_worsening) const {
        const Eval multiplier =
            threshold - kNoTtCutNodeDiscount * no_tt_cut_node;
        return multiplier * depth -
               improving * multiplier * kImprovingDepthDeduction -
               opponent_worsening * multiplier / kOpponentWorseningDivisor;
      }
    };
    struct NMPSettings {
      static constexpr bool kEnabled = true;
      static constexpr SfMargin kStaticEvalBaseMargin{365};
      static constexpr SfMargin kStaticEvalDepthMargin{13};
      static constexpr SfMargin kStaticEvalImprovingMargin{47};
      static constexpr SfMargin kMinBeta{-2000};
      static constexpr SfMargin kEvalDivisor{256};
      static constexpr int kDepthDivisor = 3;
      static constexpr Depth kVerificationDepth = 16;

      int null_move_reduction = 7;

      [[nodiscard]] static constexpr SfMargin StaticEvalMargin(Depth depth,
                                                               bool improving) {
        return kStaticEvalBaseMargin - kStaticEvalDepthMargin * depth -
               kStaticEvalImprovingMargin * improving;
      }
    };
    struct IIRSettings {
      Depth base_limit = 2;
      Depth cut_node_penalty = 1;
      Depth reduction = 1;
    };
    struct ProbCutSettings {
      static constexpr Depth kMinDepth = 5;
      static constexpr int kDepthReduction = 4;
      static constexpr SfMargin kMargin{241};
      static constexpr SfMargin kImprovingMargin = kMargin - SfMargin{64};
      static_assert(kMinDepth > kDepthReduction);
    };
    struct LMRSettings {
      static constexpr bool kEnabled = true;
      static constexpr double kNotImprovingReductionScale = 191.0 / 512;

      Depth depth_limit = 3;
      int under_check_reduction_penalty = 1;
      int doing_check_reduction_penalty = 2;
      int cut_node_reduction = 2;
      int history_divisor = 13000;

      [[nodiscard]] static constexpr double ScaleReduction(
          double reduction_scale, bool improving) {
        return reduction_scale +
               !improving * reduction_scale * kNotImprovingReductionScale;
      }
    };
    struct FutilitySettings {
      static constexpr bool kEnabled = true;

      Depth depth_limit = 6;
      Eval base_margin = 90;
      Eval depth_margin = 60;
    };
    struct LMPSettings {
      static constexpr bool kEnabled = true;

      Depth depth_limit = 8;
      int base_count = 3;

      [[nodiscard]] constexpr int MoveCount(Depth depth, bool improving) const {
        return (base_count + depth * depth) / (2 - improving);
      }
    };
    struct SEEPruningSettings {
      static constexpr bool kEnabled = true;

      Depth depth_limit = 8;
      Eval quiet_margin = 60;
      Eval noisy_margin = 20;
    };
  };
  struct SearchParameters {
    static constexpr SfMargin kImprovingBetaMargin{0};
    struct StaticEvalPolicySettings {
      static constexpr int kMinDifference = -189;
      static constexpr int kMaxDifference = 194;
      static constexpr int kOffset = 60;
      static constexpr int kBonusMultiplier = 11;
      static constexpr int kStockfishHistoryMax = 7183;
      static_assert(kMinDifference <= kMaxDifference);
    };
    struct HistorySettings {
      static constexpr std::int64_t kDecayNumerator = 729;
      static constexpr std::int64_t kDecayDenominator = 1024;
      static_assert(0 < kDecayNumerator && kDecayNumerator < kDecayDenominator);
    };
    struct SingularExtensionSettings {
      static constexpr Eval kStockfishMarginNumerator = 59;
      static constexpr Eval kStockfishMarginDenominator = 54;
      static constexpr Eval kBigNetworkDefaultScale = 311;

      static constexpr Depth kDepthLimit = 6;
      static constexpr Eval kMarginNumerator =
          kStockfishMarginNumerator * kBigNetworkDefaultScale;
      static constexpr Eval kMarginDenominator =
          kStockfishMarginDenominator * kNnueScaleDenominator;
      static constexpr int kNegativeExtension = -3;

      [[nodiscard]] static constexpr Eval Margin(Depth depth) {
        return kMarginNumerator * depth / kMarginDenominator;
      }
    };
    struct AspirationWindowSettings {
      static constexpr double kInitialDelta = SfMargin{5}.ToEngine();
      static constexpr double kThreadDelta = SfMargin{1}.ToEngine();
      static constexpr double kMeanSquaredScoreDivisor =
          SfMargin{10193}.ToEngine();
      static constexpr double kDeltaGrowth = 47.0 / 128.0;
      static_assert(kInitialDelta > 0 && kDeltaGrowth > 0);
    };
    static constexpr AspirationWindowSettings kAspirationWindowSettings = {};
  };
  static constexpr PruneParameters kPruneParameters = {};
  static constexpr SearchParameters kSearchParameters = {};

  template <class Group>
    requires(!std::is_empty_v<Group>)
  static constinit inline Group current{};
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
std::ostream& operator<<(std::ostream& out,
                         Settings::SearchParameters parameters);
std::ostream& operator<<(std::ostream& out,
                         Settings::PruneParameters parameters);
}  // namespace SimpleChessEngine
