#pragma once

#include <string>

#include "Accumulator.h"

namespace SimpleChessEngine {
class Position;
}

#ifndef SCE_EVALFILE
#define SCE_EVALFILE "nn-37f18f62d772.nnue"
#endif

#ifndef SCE_EVALFILE_SMALL
#define SCE_EVALFILE_SMALL "nn-37f18f62d772.nnue"
#endif

namespace SimpleChessEngine::Nnue {
inline constexpr const char* kDefaultEvalFile = SCE_EVALFILE;
inline constexpr const char* kDefaultEvalFileSmall = SCE_EVALFILE_SMALL;

struct NetworkOutput {
  int psqt = 0;
  int positional = 0;
};

[[nodiscard]] bool Load(const std::string& path);
[[nodiscard]] bool LoadSmall(const std::string& path);
void UnloadSmall();
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
