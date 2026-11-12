#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

#include "EnumArray.h"

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

template <>
inline constexpr std::size_t kEnumSize<Piece> = 7;

template <class Value>
using ByPiece = EnumArray<Piece, Value>;

inline constexpr std::array kPiecesByValue{Piece::kPawn, Piece::kKnight,
                                           Piece::kBishop, Piece::kRook,
                                           Piece::kQueen};
}  // namespace SimpleChessEngine
