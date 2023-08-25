#pragma once
#include <variant>

#include "BitBoard.h"
#include "Piece.h"
namespace SimpleChessEngine
{
struct DefaultMove
{
  bool operator==(const DefaultMove&) const = default;

  BitIndex from{};
  BitIndex to{};
  Piece captured_piece{};
};

struct DoublePush
{
  DoublePush(const BitIndex from, const BitIndex to) : from(from), to(to) {}

  bool operator==(const DoublePush&) const = default;

  BitIndex from{};
  BitIndex to{};
};

struct EnCroissant
{
  BitIndex from{};
  BitIndex to{};

  bool operator==(const EnCroissant&) const = default;
};

struct Promotion : DefaultMove
{
  bool operator==(const Promotion& other) const
  {
    return DefaultMove::operator==(other) && promoted_to == other.promoted_to;
  }

  Piece promoted_to{};
};

struct Castling
{
  enum class CastlingSide
  {
    k00,
    k000
  };

  CastlingSide side;

  BitIndex king_from{};
  BitIndex rook_from{};

  bool operator==(const Castling&) const = default;
};

using Move =
    std::variant<DefaultMove, DoublePush, EnCroissant, Promotion, Castling>;
}  // namespace SimpleChessEngine