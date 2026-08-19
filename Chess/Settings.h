#pragma once
#include <ostream>

#include "Evaluation.h"
#include "Utility.h"

namespace SimpleChessEngine {
struct Settings {
  struct EvaluationParameters {
    inline static std::array<TaperedEval, kPieceTypes> mobility = [] {
      std::array<TaperedEval, kPieceTypes> values{};
      values[static_cast<size_t>(Piece::kKnight)] = TaperedEval{{4, 2}};
      values[static_cast<size_t>(Piece::kBishop)] = TaperedEval{{7, 3}};
      values[static_cast<size_t>(Piece::kRook)] = TaperedEval{{2, 4}};
      values[static_cast<size_t>(Piece::kQueen)] = TaperedEval{{2, 2}};
      return values;
    }();
    inline static std::array<std::array<std::array<TaperedEval, 4>, 8>,
                             kPieceTypes>
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
    inline static std::array<TaperedEval, kPieceTypes> material_value = [] {
      auto values = kPieceValues;
      values[static_cast<size_t>(Piece::kKnight)]
          .eval[static_cast<size_t>(GamePhase::kEndGame)] = 275;
      values[static_cast<size_t>(Piece::kBishop)]
          .eval[static_cast<size_t>(GamePhase::kMiddleGame)] = 375;
      return values;
    }();
    inline static Eval king_shield_near = 12;
    inline static Eval king_shield_far = 10;
    inline static Eval king_semi_open_file = -14;
    inline static Eval king_open_file = -14;
    inline static Eval king_pawn_storm_near = -12;
    inline static Eval king_pawn_storm_far = -8;
    inline static std::array<Eval, kPieceTypes> king_attack = [] {
      std::array<Eval, kPieceTypes> values{};
      values[static_cast<size_t>(Piece::kKnight)] = 8;
      values[static_cast<size_t>(Piece::kBishop)] = 10;
      values[static_cast<size_t>(Piece::kRook)] = 18;
      values[static_cast<size_t>(Piece::kQueen)] = 20;
      return values;
    }();
    inline static bool pawns_enabled = true;
    inline static bool material_enabled = true;
    inline static bool mobility_enabled = true;
    inline static bool psqt_adjustment_enabled = false;
    inline static bool king_pawns_enabled = true;
    inline static bool king_attacks_enabled = true;
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
      material_value[static_cast<size_t>(piece)]
          .eval[static_cast<size_t>(phase)] = value;
      material_enabled = material_value != kPieceValues;
    }

    static void SetMobility(Piece piece, GamePhase phase, Eval value) {
      mobility[static_cast<size_t>(piece)].eval[static_cast<size_t>(phase)] =
          value;
      mobility_enabled = false;
      for (const auto& weight : mobility) {
        mobility_enabled |= weight != TaperedEval{};
      }
    }

    static void SetPsqtAdjustment(Piece piece, size_t relative_rank,
                                  size_t mirrored_file, GamePhase phase,
                                  Eval value) {
      psqt_adjustment[static_cast<size_t>(piece)][relative_rank][mirrored_file]
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
      king_attack[static_cast<size_t>(piece)] = value;
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
