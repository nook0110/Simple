#include "Attacks.h"

using namespace SimpleChessEngine;

template<Piece SlidingPiece>
[[nodiscard]] Bitboard<> GenerateAttackMask(const BitIndex square, const Bitboard<> occupancy = kBorderBB)
{
    static_assert(SlidingPiece == Piece::kBishop || SlidingPiece == Piece::kRook);
    assert(IsOk(square));
    if constexpr (SlidingPiece == Piece::kBishop)
    {
        static constexpr delta = { Compass::kNorthWest, Compass::kSouthWest, Compass::kSouthEast, Compass::kNorthEast };
    }
    if constexpr (SlidingPiece == Piece::kRook)
    {
        static constexpr delta = { Compass::kNorth, Compass::kWest, Compass::kSouth, Compass::kEast };
    }
    Bitboard<> result = kEmptyBoard;
    for (size_t i = 0; i < kMoveDirections; ++i)
    {
        for (BitIndex temp = square; true; temp += delta[i])
        {
            auto square_bb = GetBitboardOfSquare(temp);
            result |= square_bb;
            if (square_bb & occupancy != kEmptyBoard) break;
        }
    }
    result ^= GetBitboardOfSquare(square);
    return result;
}

void AttackTable::InitBishopMagics()
{}

void AttackTable::InitRookMagics()
{}

void AttackTable::Init()
{
    InitBishopMagics();
    InitRookMagics();
}
