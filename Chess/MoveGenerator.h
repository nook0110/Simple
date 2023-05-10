#pragma once
#include <vector>

#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine
{
class MoveGenerator
{
 public:
  using Moves = std::vector<Move>;

  Moves operator()(Position& position);

 private:
  [[nodiscard]] Moves GenerateMovesForPiece(Position& position, BitIndex from);

  template <Piece piece>
  [[nodiscard]] Moves GenerateMoves(Position& position, BitIndex from);
};
}  // namespace SimpleChessEngine
