#pragma once
#include "BitScan.h"

template <size_t Size = 64>
class BitBoard : public std::bitset<Size>
{
 public:
  using std::bitset<Size>::bitset;
  static_assert(Size <= 64);

  [[nodiscard]] BitIndex GetFirstBit() const;
};

template <size_t Size>
BitIndex BitBoard<Size>::GetFirstBit() const
{
  return BitScan(this->to_ullong());
}

constexpr BitBoard<64> kEmptyBoard = 0;
