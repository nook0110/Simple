#pragma once

#include <string>

#include "Accumulator.h"

namespace SimpleChessEngine {
class Position;
}

#ifndef SCE_EVALFILE
#define SCE_EVALFILE "nn-37f18f62d772.nnue"
#endif

namespace SimpleChessEngine::Nnue {
inline constexpr const char* kDefaultEvalFile = SCE_EVALFILE;

struct NetworkOutput {
  int psqt = 0;
  int positional = 0;
};

[[nodiscard]] bool Load(const std::string& path);
void Unload();
[[nodiscard]] bool IsLoaded();
[[nodiscard]] std::string Description();
[[nodiscard]] int DefaultScale();
[[nodiscard]] std::uint32_t Generation();

[[nodiscard]] NetworkOutput Evaluate(const Position& position,
                                     AccumulatorStack& stack);
[[nodiscard]] NetworkOutput EvaluateFromScratch(const Position& position);
[[nodiscard]] bool VerifyAccumulators(const Position& position,
                                      AccumulatorStack& stack);
}  // namespace SimpleChessEngine::Nnue
