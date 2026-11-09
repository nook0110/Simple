#pragma once

#include <array>
#include <cstdint>

#include "Utility.h"

namespace SimpleChessEngine {
using Eval = int;
using SearchResult = std::optional<Eval>;

enum class GamePhase : std::uint8_t { kMiddleGame, kEndGame };

template <>
inline constexpr std::size_t kEnumSize<GamePhase> = 2;

template <class Value>
using ByGamePhase = EnumArray<GamePhase, Value>;

constexpr size_t kGamePhases = 2;

using PhaseValue = int;

struct TaperedEval {
  std::array<Eval, kGamePhases> eval{};
  [[nodiscard]] Eval operator()(PhaseValue pv) const;
  [[nodiscard]] bool operator==(const TaperedEval& other) const = default;

  [[maybe_unused]] TaperedEval& operator+=(const TaperedEval& other) {
    eval[0] += other.eval[0];
    eval[1] += other.eval[1];
    return *this;
  }
  [[maybe_unused]] TaperedEval& operator-=(const TaperedEval& other) {
    eval[0] -= other.eval[0];
    eval[1] -= other.eval[1];
    return *this;
  }
};

[[nodiscard]] inline TaperedEval operator+(const TaperedEval& lhs,
                                           const TaperedEval& rhs) {
  auto copy = lhs;
  copy += rhs;
  return copy;
}

[[nodiscard]] inline TaperedEval operator-(const TaperedEval& lhs,
                                           const TaperedEval& rhs) {
  auto copy = lhs;
  copy -= rhs;
  return copy;
}

constexpr ByPiece<TaperedEval> kPieceValues = {{{{0, 0},
                                                                {82, 94},
                                                                {337, 281},
                                                                {365, 297},
                                                                {477, 512},
                                                                {1025, 936},
                                                                {0, 0}}}};

constexpr Eval kFullNonPawnMaterial =
    kPieceValues[Piece::kKnight]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        4 +
    kPieceValues[Piece::kBishop]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        4 +
    kPieceValues[Piece::kRook]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        4 +
    kPieceValues[Piece::kQueen]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        2;

constexpr ByGamePhase<PhaseValue> kPhaseValueLimits = {{kFullNonPawnMaterial, 0}};

constexpr PhaseValue kLimitsDifference =
    kPhaseValueLimits[GamePhase::kMiddleGame] -
    kPhaseValueLimits[GamePhase::kEndGame];

constexpr Eval kTempoBonus = 20;

constexpr Eval kNnueScaleDenominator = 1024;
constexpr Eval kNnueEvalLimit = 20'000;

constexpr Eval kMateValue = -100'000;
constexpr Eval kDrawValue = 0;
constexpr Eval kTablebaseWinValue = 50'000;

// returns zero if the score is not mate value
// otherwise returns 1 if it is winning (positive), -1 if losing (negative)
inline int IsMateScore(const int score) {
  if (-std::abs(score) > kMateValue + static_cast<Eval>(kMaxSearchPly) + 1) {
    return 0;
  }
  return (score > 0) - (score < 0);
}
}  // namespace SimpleChessEngine
