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

template <Piece SlidingPiece, size_t table_size>
AttackTable<SlidingPiece, table_size>::AttackTable()
{
    if constexpr (SlidingPiece != Piece::kBishop && SlidingPiece != Piece::kRook) return;
    std::vector<Bitboard<>> occupancy(1 << 16), reference(1 << 16);
    std::vector<int> epoch(1 << 16);
    unsigned attempt_count = 0;
    size_t offset = 0;
    Bitboard<> rank_edges = kRankBitboard.front() | kRankBitboard.back();
    Bitboard<> file_edges = kFileBitboard.front() | kFileBitboard.back();
    for (BitIndex sq = 0; sq < kBoardArea; ++sq)
    {
        auto [file, rank] = GetCoordinates(sq);
        Bitboard<> edges = (rank_edges & ~kRankBitboard[rank]) | (file_edges & ~kFileBitboard[file]);
        magic[sq].mask = GenerateAttackMask<SlidingPiece>(sq) & ~edges;
        magic[sq].shift = 64 - magic[sq].mask.count();
        base[sq] = (sq ? base[sq - 1] + offset : 0);
        offset = 0;
        Bitboard<> b(kEmptyBoard);
        do
        {
            occupancy[offset] = b;
            reference[offset] = GenerateAttackMask<SlidingPiece>(sq, b);
            ++offset;
            b = (b - magic[sq].mask) & magic[sq].mask;
        } while (b.any());
        std::mt19937_64 gen(std::chrono::steady_clock::now().time_since_epoch().count());
        for (size_t i = 0; i < offset; )
        {
            for (magic[sq].magic = 0; (Bitboard<>(magic[sq].magic * magic[sq].mask.to_ullong()) >> 56).count() < 6; )
                magic[sq].magic = gen();
            ++attempt_count;
            for (i = 0; i < offset; ++i)
            {
                auto idx = GetAttackTableAddress(sq, occupancy[i]);
                if (epoch[idx] < attempt_count)
                {
                    epoch[idx] = attempt_count;
                    table[base[sq] + idx] = reference[i];
                }
                else if (table[base[sq] + idx] != reference[i])
                    break;
            }
        }
    }
}
