#pragma once
#include <cstdint>
#include <tuple>
#include <variant>

#include "BitBoard.h"
#include "Piece.h"
namespace SimpleChessEngine {
struct DefaultMove {
  bool operator==(const DefaultMove&) const = default;

  BitIndex from : 6 {};
  BitIndex to : 6 {};
  Piece captured_piece : 3 {};
};

struct PawnPush {
  bool operator==(const PawnPush&) const = default;

  BitIndex from : 6 {};
  BitIndex to : 6 {};
};

struct DoublePush {
  bool operator==(const DoublePush&) const = default;

  BitIndex from : 6 {};
  BitIndex to : 6 {};
};

struct EnCroissant {
  BitIndex from : 6 {};
  BitIndex to : 6 {};

  bool operator==(const EnCroissant&) const = default;
};

struct Promotion : DefaultMove {
  bool operator==(const Promotion& other) const {
    return DefaultMove::operator==(other) && promoted_to == other.promoted_to;
  }

  Piece promoted_to : 3 {};
};

struct Castling {
  enum class CastlingSide : uint8_t { k00, k000 };

  BitIndex king_from : 6 {};
  BitIndex rook_from : 6 {};
  CastlingSide side : 1;

  bool operator==(const Castling&) const = default;
};

using Move = std::variant<PawnPush, DoublePush, EnCroissant, DefaultMove,
                          Castling, Promotion>;

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const PawnPush& move) {
  return {move.from, move.to, Piece::kNone};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(
    const DoublePush& move) {
  return {move.from, move.to, Piece::kNone};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(
    const EnCroissant& move) {
  return {move.from, move.to, Piece::kPawn};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(
    const DefaultMove& move) {
  return {move.from, move.to, move.captured_piece};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const Castling& move) {
  return {move.king_from, 64, Piece::kNone};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(
    const Promotion& move) {
  return {move.from, move.to, move.captured_piece};
}

inline std::tuple<BitIndex, BitIndex, Piece> GetMoveData(const Move& move) {
  return std::visit(
      [](const auto& unwrapped_move) { return GetMoveData(unwrapped_move); },
      move);
}
}  // namespace SimpleChessEngine
