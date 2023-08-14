#pragma once
#include "BitBoard.h"
#include "Piece.h"
namespace SimpleChessEngine
{
struct Move
{
  using Index = BitIndex;

  Move() = default;

  bool operator==(const Move& other) const { return false; }
  bool operator<(const Move&) const { return false; }

  Index from{};
  Index to{};
  Piece captured_piece{};
};
}  // namespace SimpleChessEngine