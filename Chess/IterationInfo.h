#pragma once
#include "Evaluation.h"

namespace SimpleChessEngine {
class Searcher;

struct IterationInfo {
  const Searcher& searcher;
  Eval iteration_result;
  Depth depth;
};
}  // namespace SimpleChessEngine
