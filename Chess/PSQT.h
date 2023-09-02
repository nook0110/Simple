#pragma once

#include "Evaluation.h"

namespace SimpleChessEngine
{
  std::array<std::array<TaperedEval, kBoardArea>, kPieceTypes> kPSQT =
  {
    {
      // Empty square value is irrelevant since kNone is not being placed or removed at any time
      {},
      //
      {}
    }
  };
}