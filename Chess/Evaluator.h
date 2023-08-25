#pragma once
#include <cassert>

#include "MoveGenerator.h"
#include "Position.h"

namespace SimpleChessEngine
{
using Eval = int;

enum class GamePhase
{
  kMG,
  kEG
};

constexpr size_t kGamePhases = 2;

using PhaseValue = int;

constexpr std::array<PhaseValue, kGamePhases> kPhaseValueLimits = {};

struct TaperedEval
{
  std::array<Eval, kGamePhases> eval;
  [[nodiscard]] Eval operator()(PhaseValue pv) const
  {
    if (pv < kPhaseValueLimits[0]) pv = kPhaseValueLimits[0];
    if (pv > kPhaseValueLimits[1]) pv = kPhaseValueLimits[1];
    return eval[0] * (kPhaseValueLimits[1] - pv) +
           eval[1] * (pv - kPhaseValueLimits[0]);
  }
};

class Evaluator
{
 public:
  Eval operator()(const Position& position, Eval alpha, Eval beta) const;

  [[nodiscard]] Eval GetGameResult(const Position& position) const
  {
    // assert(MoveGenerator{}(const_cast<Position&>(position)).empty());
    return Eval{};
  }
};

inline Eval Evaluator::operator()(const Position& position, Eval alpha,
                                  Eval beta) const
{
  return Eval{};
}
}  // namespace SimpleChessEngine
