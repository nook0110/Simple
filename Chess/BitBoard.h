#pragma once
#include "BitScan.h"

/**
 * \brief Class that represents a bitboard.
 *
 * \details A simple wrapper of std::bitset<size>.
 *
 * \tparam size The size of the bitboard.
 *
 * \author nook0110
 */
template <size_t size = 64>
class Bitboard : public std::bitset<size>
{
 public:
  using std::bitset<size>::bitset;
  static_assert(size <= 64);

  /**
   * \brief Constructs a bitboard from a bitset.
   */
  explicit Bitboard(std::bitset<size> bitset)
      : std::bitset<size>(std::move(bitset))
  {}

  [[nodiscard]] BitIndex GetFirstBit() const;
};

template <size_t Size>
BitIndex Bitboard<Size>::GetFirstBit() const
{
  return BitScan(this->to_ullong());
}

constexpr Bitboard<64> kEmptyBoard = 0;
