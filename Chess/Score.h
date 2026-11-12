#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "Evaluation.h"
#include "Utility.h"

namespace SimpleChessEngine::Score {
inline constexpr Eval kMate = 100'000;
inline constexpr Eval kTbWin = 50'000;
inline constexpr Eval kDecisive = kTbWin - static_cast<Eval>(kMaxSearchPly);

[[nodiscard]] constexpr Eval MateIn(const Depth ply) { return kMate - ply; }
[[nodiscard]] constexpr Eval MatedIn(const Depth ply) { return -kMate + ply; }
[[nodiscard]] constexpr Eval TbWinIn(const Depth ply) { return kTbWin - ply; }
[[nodiscard]] constexpr Eval TbLossIn(const Depth ply) { return -kTbWin + ply; }

[[nodiscard]] constexpr bool IsWin(const Eval value) {
  return value >= kDecisive;
}
[[nodiscard]] constexpr bool IsLoss(const Eval value) {
  return value <= -kDecisive;
}
[[nodiscard]] constexpr bool IsDecisive(const Eval value) {
  return IsWin(value) || IsLoss(value);
}

inline constexpr Eval kMateThreshold =
    MateIn(static_cast<Depth>(kMaxSearchPly + 1));

[[nodiscard]] constexpr bool IsMateWin(const Eval value) {
  return value >= kMateThreshold;
}
[[nodiscard]] constexpr bool IsMateLoss(const Eval value) {
  return value <= -kMateThreshold;
}
[[nodiscard]] constexpr bool IsMate(const Eval value) {
  return IsMateWin(value) || IsMateLoss(value);
}
[[nodiscard]] constexpr int MovesToMate(const Eval value) {
  return (value > 0 ? 1 : -1) * ((kMate - std::abs(value) + 1) / 2);
}
}  // namespace SimpleChessEngine::Score

namespace SimpleChessEngine {
[[nodiscard]] constexpr std::int64_t DivideRounded(
    const std::int64_t numerator, const std::int64_t denominator) {
  return numerator < 0 ? -((-numerator + denominator / 2) / denominator)
                       : (numerator + denominator / 2) / denominator;
}

class SfMargin {
 public:
  static constexpr Eval kStockfishPawn = 208;
  static constexpr Eval kEnginePawn =
      kPieceValues[Piece::kPawn]
          .eval[static_cast<std::size_t>(GamePhase::kMiddleGame)];
  static constexpr double kPerEngineUnit =
      static_cast<double>(kStockfishPawn) / kEnginePawn;

  constexpr explicit SfMargin(const Eval stockfish_units)
      : value_(stockfish_units) {}

  [[nodiscard]] static constexpr std::int64_t RoundedFromEngine(
      const std::int64_t engine_units) {
    return DivideRounded(engine_units * kStockfishPawn, kEnginePawn);
  }

  [[nodiscard]] constexpr double ToEngine() const {
    return value_ * kEnginePawn / static_cast<double>(kStockfishPawn);
  }

  [[nodiscard]] constexpr Eval RoundedToEngine() const {
    return static_cast<Eval>(DivideRounded(
        static_cast<std::int64_t>(value_) * kEnginePawn, kStockfishPawn));
  }

  friend constexpr bool operator==(SfMargin, SfMargin) = default;

  [[nodiscard]] friend constexpr SfMargin operator+(const SfMargin lhs,
                                                    const SfMargin rhs) {
    return SfMargin{lhs.value_ + rhs.value_};
  }
  [[nodiscard]] friend constexpr SfMargin operator-(const SfMargin lhs,
                                                    const SfMargin rhs) {
    return SfMargin{lhs.value_ - rhs.value_};
  }
  [[nodiscard]] friend constexpr SfMargin operator*(const SfMargin margin,
                                                    const int factor) {
    return SfMargin{margin.value_ * factor};
  }

  [[nodiscard]] friend constexpr std::strong_ordering operator<=>(
      const Eval engine_units, const SfMargin margin) {
    return static_cast<std::int64_t>(engine_units) * kStockfishPawn <=>
           static_cast<std::int64_t>(margin.value_) * kEnginePawn;
  }

 private:
  Eval value_;
};
}  // namespace SimpleChessEngine
