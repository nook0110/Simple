#pragma once

#include "Position.h"

namespace SimpleChessEngine
{
template <bool print = true>
size_t Perft(std::ostream& o_stream, Position& position, size_t depth);
}  // namespace SimpleChessEngine
