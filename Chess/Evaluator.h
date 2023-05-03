#pragma once
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
    return Eval{};
  };
};
}  // namespace SimpleChessEngine
