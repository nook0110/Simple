#pragma once
#include <variant>

#include "BitBoard.h"
#include "Piece.h"
namespace SimpleChessEngine
{
struct DefaultMove
{
  BitIndex from{};
  BitIndex to{};
  Piece captured_piece{};
};

struct DoublePush
{
  BitIndex from{};
};

struct EnCroissant
{
  BitIndex from{};
  BitIndex to{};
};

struct Promotion : DefaultMove
{
  Piece promoted_to{};
};

struct Castling
{};

using Move = std::variant<DefaultMove, DoublePush, EnCroissant, Promotion, Castling>;
}  // namespace SimpleChessEngine