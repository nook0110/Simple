#pragma once

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
