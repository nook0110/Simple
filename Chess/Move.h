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

  bool operator==(const DefaultMove&) const = default;
};

struct DoublePush
{
  BitIndex from{};
};

struct EnCroissant
{
  BitIndex from{};
  BitIndex to{};

  bool operator==(const EnCroissant&) const = default;
};

struct Promotion : DefaultMove
{
  Piece promoted_to{};

  bool operator==(const Promotion& other) const
  {
    return DefaultMove::operator==(other) && promoted_to == other.promoted_to;
  }
};

struct Castling
{
  bool operator==(const Castling&) const = default;
};

using Move = std::variant<DefaultMove, DoublePush, EnCroissant, Promotion, Castling>;
}  // namespace SimpleChessEngine