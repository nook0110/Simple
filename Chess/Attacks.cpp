#include "Attacks.h"

using namespace SimpleChessEngine;

template <Piece SlidingPiece>
constexpr std::array<Compass, 4> GetDelta()
{
  if constexpr (SlidingPiece == Piece::kBishop)
  {
    return {Compass::kNorthWest, Compass::kSouthWest, Compass::kSouthEast,
            Compass::kNorthEast};
  }
  if constexpr (SlidingPiece == Piece::kRook)
  {
    return {Compass::kNorth, Compass::kWest, Compass::kSouth, Compass::kEast};
  }

  assert(false);
  return {};
}

template <Piece SlidingPiece>
[[nodiscard]] Bitboard<> GenerateAttackMask(
    const BitIndex square, const Bitboard<> occupancy = kBorderBB)
{
  static_assert(SlidingPiece == Piece::kBishop || SlidingPiece == Piece::kRook);
  assert(IsOk(square));

  static constexpr auto delta = GetDelta<SlidingPiece>();

  Bitboard<> result = kEmptyBoard;

  for (size_t i = 0; i < kMoveDirection; ++i)
  {
    for (BitIndex temp = square; true; temp += delta[i])
    {
      auto square_bb = GetBitboardOfSquare(temp);
      result |= square_bb;
      if ((square_bb & occupancy) != kEmptyBoard) break;
    }
  }
  result ^= GetBitboardOfSquare(square);
  return result;
}

void AttackTable::InitBishopMagics() {}

void AttackTable::InitRookMagics() {}

void AttackTable::Init()
{
  InitBishopMagics();
  InitRookMagics();
}