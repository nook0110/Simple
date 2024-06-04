#pragma once
#include <cstdint>

namespace SimpleChessEngine
{
/**
 * \brief Enum class that represents a piece.
 */
enum class Piece : uint8_t
{
  kNone,    //!< No piece.
  kPawn,    //!< Pawn.
  kKnight,  //!< Knight.
  kBishop,  //!< Bishop.
  kRook,    //!< Rook.
  kQueen,   //!< Queen.
  kKing     //!< King.
};

[[nodiscard]] constexpr bool operator!(const Piece piece)
{
  return piece == Piece::kNone;
}
}  // namespace SimpleChessEngine