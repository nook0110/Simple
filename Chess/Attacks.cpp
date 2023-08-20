#include "Attacks.h"

using namespace SimpleChessEngine;

template <Piece sliding_piece>
constexpr std::array<Compass, 4> GetStepDelta()
{
  if constexpr (sliding_piece == Piece::kBishop)
  {
    return {Compass::kNorthWest, Compass::kSouthWest, Compass::kSouthEast,
            Compass::kNorthEast};
  }
  if constexpr (sliding_piece == Piece::kRook)
  {
    return {Compass::kNorth, Compass::kWest, Compass::kSouth, Compass::kEast};
  }

  assert(false);
  return {};
}

template <Piece sliding_piece>
[[nodiscard]] Bitboard<> GenerateAttackMask(
    const BitIndex square, const Bitboard<> occupancy = kEmptyBoard)
{
  static_assert(IsWeakSlidingPiece(sliding_piece));
  assert(IsOk(square));

  Bitboard<> result = kEmptyBoard;

  for (auto direction : GetStepDelta<sliding_piece>())
  {
    const auto step = static_cast<int>(direction);
    for (BitIndex temp = square; IsValidShift(temp, direction) &&
                                 (occupancy & GetBitboardOfSquare(temp)).none();
         result |= GetBitboardOfSquare(temp += step))
      ;
  }
  return result;
}

template <Piece sliding_piece, size_t table_size>
AttackTable<sliding_piece, table_size>::AttackTable()
{
  if constexpr (!IsWeakSlidingPiece(sliding_piece))
    return;

  std::vector<Bitboard<>> occupancy(1 << 16), reference(1 << 16);
  std::vector<int> epoch(1 << 16);

  unsigned attempt_count = 0;
  size_t offset = 0;

  const Bitboard<> rank_edges = kRankBitboard.front() | kRankBitboard.back();
  const Bitboard<> file_edges = kFileBitboard.front() | kFileBitboard.back();

  for (BitIndex sq = 0; sq < kBoardArea; ++sq)
  {
    auto [file, rank] = GetCoordinates(sq);
    Bitboard<> edges =
        rank_edges & ~kRankBitboard[rank] | file_edges & ~kFileBitboard[file];
    magic_[sq].mask = GenerateAttackMask<sliding_piece>(sq) & ~edges;
    magic_[sq].shift = 64 - magic_[sq].mask.count();
    base_[sq] = (sq ? base_[sq - 1] + offset : 0);
    offset = 0;
    Bitboard b(kEmptyBoard);
    do
    {
      occupancy[offset] = b;
      reference[offset] = GenerateAttackMask<sliding_piece>(sq, b);
      ++offset;
      b = Bitboard<>{b.to_ullong() - magic_[sq].mask.to_ullong()} &
          magic_[sq].mask;
    }
    while (b.any());
    std::mt19937_64 gen(
        std::chrono::steady_clock::now().time_since_epoch().count());
    for (size_t i = 0; i < offset;)
    {
      for (magic_[sq].magic = 0;
           (Bitboard<>(magic_[sq].magic * magic_[sq].mask.to_ullong()) >> 56)
               .count() < 6;)
        magic_[sq].magic = gen() & gen() & gen();
      ++attempt_count;
      for (i = 0; i < offset; ++i)
      {
        const auto idx = magic_[sq].GetIndex(occupancy[i]);

        if (epoch[idx] < attempt_count)
        {
          epoch[idx] = attempt_count;
          table_[base_[sq] + idx] = reference[i];
        }
        else if (table_[base_[sq] + idx] != reference[i])
          break;
      }
    }
  }
}
