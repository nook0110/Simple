#pragma once
#include <cassert>

#include "MoveGenerator.h"
#include "Position.h"

namespace SimpleChessEngine
{
using Eval = int;

class Evaluator
{
 public:
  Eval operator()(const Position& position, Eval alpha, Eval beta) const
  {
    return Eval{};
  };

  [[nodiscard]] Eval GetGameResult(const Position& position) const
  {
    assert(MoveGenerator{}(const_cast<Position&>(position)).empty());
    return Eval{};
  };
};
}  // namespace SimpleChessEngine
