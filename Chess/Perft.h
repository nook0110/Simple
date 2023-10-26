#pragma once
#include <ostream>

#include "MoveGenerator.h"
#include "Position.h"
#include "StreamUtility.h"

namespace SimpleChessEngine
{
template <bool print = true>
size_t Perft(std::ostream& o_stream, Position& position, const size_t depth);
}  // namespace SimpleChessEngine
