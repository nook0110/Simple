#pragma once
#include <cassert>

#include "Position.h"

namespace SimpleChessEngine
{
using Eval = int;

class Evaluator
{
 public:
  Eval operator()(const Position& position, Eval alpha, Eval beta) const;

  [[nodiscard]] Eval GetGameResult(const Position& position) const
  {
    assert(MoveGenerator{}(const_cast<Position&>(position)).empty());
    return Eval{};
  }
};

inline Eval Evaluator::operator()(const Position& position, Eval alpha,
                                  Eval beta) const
{
  return Eval{};
}
}  // namespace SimpleChessEngine
