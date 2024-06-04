#pragma once
#include <cstdint>
#include <tuple>
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

struct PawnPush
{
  bool operator==(const PawnPush&) const = default;

  BitIndex from{};
  BitIndex to{};
};

struct DoublePush
{
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
  enum class CastlingSide : uint8_t
  {
    k00,
    k000
  };

  CastlingSide side;

  BitIndex king_from{};
  BitIndex rook_from{};

  bool operator==(const Castling&) const = default;
};

using Move = std::variant<PawnPush, DoublePush, EnCroissant, DefaultMove,
                          Castling, Promotion>;

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const PawnPush& move)
{
  return {move.from, move.to, Piece::kNone};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const DoublePush& move)
{
  return {move.from, move.to, Piece::kNone};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(
    const EnCroissant& move)
{
  return {move.from, move.to, Piece::kPawn};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(
    const DefaultMove& move)
{
  return {move.from, move.to, move.captured_piece};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const Castling& move)
{
  return {move.king_from, 64, Piece::kNone};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const Promotion& move)
{
  return {move.from, move.to, move.captured_piece};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const Move& move)
{
  return std::visit([](const auto& unwrapped_move)
                    { return GetMoveData(unwrapped_move); },
                    move);
}

inline bool IsQuiet(const Move& move) {
  return !std::get<Piece>(GetMoveData(move));
}
}  // namespace SimpleChessEngine