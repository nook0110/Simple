#pragma once

#include "AlphaBeta.h"
#include "Evaluator.h"
#include "HashTable.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "Position.h"

namespace SimpleChessEngine
{
using ChessEngine =
    AlphaBetaSearcher<Position, MoveGenerator, Evaluator, HashTable<100>>;
}  // namespace SimpleChessEngine
