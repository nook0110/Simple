#pragma once

#include "Position.h"

namespace SimpleChessEngine {
template <bool print = true>
size_t Perft(std::ostream& o_stream, Position& position, Depth depth);

struct PerftResult {
  size_t nodes;
  size_t nps;
};

PerftResult PerftBench(Position& position, Depth depth);
}  // namespace SimpleChessEngine
