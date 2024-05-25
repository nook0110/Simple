#pragma once

#include <array>
#include <cstdint>

#include "Utility.h"

namespace SimpleChessEngine {
using Eval = int;

enum class GamePhase : uint8_t { kMiddleGame, kEndGame };

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

constexpr std::array<TaperedEval, kPieceTypes> kPieceValues = {{{0, 0},
                                                                {100, 140},
                                                                {330, 330},
                                                                {330, 360},
                                                                {600, 650},
                                                                {1000, 1150},
                                                                {0, 0}}};

constexpr Eval kFullNonPawnMaterial =
    kPieceValues[static_cast<size_t>(Piece::kKnight)]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        4 +
    kPieceValues[static_cast<size_t>(Piece::kBishop)]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        4 +
    kPieceValues[static_cast<size_t>(Piece::kRook)]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        4 +
    kPieceValues[static_cast<size_t>(Piece::kQueen)]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
        2;

constexpr std::array kPhaseValueLimits = {
    kFullNonPawnMaterial -
        kPieceValues[static_cast<size_t>(Piece::kKnight)]
                .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
            2,
    kPieceValues[static_cast<size_t>(Piece::kRook)]
                .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
            2 +
        kPieceValues[static_cast<size_t>(Piece::kBishop)]
                .eval[static_cast<size_t>(GamePhase::kMiddleGame)] *
            2};

constexpr PhaseValue kLimitsDifference =
    kPhaseValueLimits[0] - kPhaseValueLimits[1];

constexpr Eval kTempoBonus = 20;

constexpr Eval kMateValue = -1e5;
}  // namespace SimpleChessEngine