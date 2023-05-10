#pragma once
#include "BitScan.h"

template <size_t Size = 64>
class Bitboard : public std::bitset<Size>
{
 public:
  using std::bitset<Size>::bitset;
  static_assert(Size <= 64);

  [[nodiscard]] BitIndex GetFirstBit() const;
};

template <size_t Size>
BitIndex Bitboard<Size>::GetFirstBit() const
{
  return BitScan(this->to_ullong());
}

constexpr Bitboard<64> kEmptyBoard = 0;
