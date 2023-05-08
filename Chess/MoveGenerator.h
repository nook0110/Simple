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

  Moves operator()(const Position& position) const;

 private:
  [[nodiscard]] Moves GenerateMovesForPiece(const Position& position,
                                            BitIndex from) const;
  [[nodiscard]] Moves GeneratePawnMoves(const Position& position,
                                        BitIndex from) const;
  [[nodiscard]] Moves GenerateKnightMoves(const Position& position,
                                          BitIndex from) const;
  [[nodiscard]] Moves GenerateBishopMoves(const Position& position,
                                          BitIndex from) const;
  [[nodiscard]] Moves GenerateRookMoves(const Position& position,
                                        BitIndex from) const;
  [[nodiscard]] Moves GenerateQueenMoves(const Position& position,
                                         BitIndex from) const;
  [[nodiscard]] Moves GenerateKingMoves(const Position& position,
                                        BitIndex from) const;
};
}  // namespace SimpleChessEngine
