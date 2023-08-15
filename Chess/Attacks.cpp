#include "Attacks.h"

using namespace SimpleChessEngine;

template <Piece SlidingPiece>
constexpr std::array<Compass, 4> GetStepDelta()
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
    const BitIndex square, const Bitboard<> occupancy = kEmptyBoard)
{
  static_assert(SlidingPiece == Piece::kBishop || SlidingPiece == Piece::kRook);
  assert(IsOk(square));

  Bitboard<> result = kEmptyBoard;

  for (auto direction : GetStepDelta<SlidingPiece>())
  {
    const auto step = static_cast<int>(direction);
    for (BitIndex temp = square; 
         IsValidShift(temp, direction) && (occupancy & GetBitboardOfSquare(temp)).none();
         result |= GetBitboardOfSquare(temp += step))
        ;
  }
  return result;
}

void AttackTable::InitBishopMagics() {}

void AttackTable::InitRookMagics() {}

void AttackTable::Init()
{
  InitBishopMagics();
  InitRookMagics();
}
