#pragma once
#include <optional>

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
  explicit constexpr Bitboard(const std::bitset<size> bitset)
      : std::bitset<size>(std::move(bitset))
  {}

  [[nodiscard]] std::optional<BitIndex> GetFirstBit() const;

  constexpr Bitboard operator&(const Bitboard& other) const;
  constexpr Bitboard& operator&=(const Bitboard& other);

  constexpr Bitboard operator|(const Bitboard& other) const;
  constexpr Bitboard& operator|=(const Bitboard& other);

  constexpr Bitboard operator^(const Bitboard& other) const;
  constexpr Bitboard& operator^=(const Bitboard& other);

  constexpr Bitboard operator~() const;

  constexpr Bitboard operator<<(size_t pos) const;
  constexpr Bitboard& operator<<=(size_t pos);

  constexpr Bitboard operator>>(size_t pos) const;
  constexpr Bitboard& operator>>=(size_t pos);
};

template <size_t size>
std::optional<BitIndex> Bitboard<size>::GetFirstBit() const
{
  return BitScan(this->to_ullong());
}

template <size_t size>
constexpr Bitboard<size> Bitboard<size>::operator&(const Bitboard& other) const
{
  auto copy = *this;
  return copy &= other;
}

template <size_t size>
constexpr Bitboard<size>& Bitboard<size>::operator&=(const Bitboard& other)
{
  std::bitset<size>::operator&=(other);
  return *this;
}

template <size_t size>
constexpr Bitboard<size> Bitboard<size>::operator|(const Bitboard& other) const
{
  auto copy = *this;
  return copy |= other;
}

template <size_t size>
constexpr Bitboard<size>& Bitboard<size>::operator|=(const Bitboard& other)
{
  std::bitset<size>::operator|=(other);
  return *this;
}

template <size_t size>
constexpr Bitboard<size> Bitboard<size>::operator^(const Bitboard& other) const
{
  auto copy = *this;
  return copy ^= other;
}

template <size_t size>
constexpr Bitboard<size>& Bitboard<size>::operator^=(const Bitboard& other)
{
  Bitboard{std::bitset<size>::operator^=(other)};
  return *this;
}

template <size_t size>
constexpr Bitboard<size> Bitboard<size>::operator~() const
{
  return Bitboard{std::bitset<size>::operator~()};
}

template <size_t size>
constexpr Bitboard<size> Bitboard<size>::operator<<(size_t pos) const
{
  return Bitboard{std::bitset<size>::operator<<(pos)};
}

template <size_t size>
constexpr Bitboard<size>& Bitboard<size>::operator<<=(size_t pos)
{
  Bitboard{std::bitset<size>::operator<<=(pos)};
  return *this;
}

template <size_t size>
constexpr Bitboard<size> Bitboard<size>::operator>>(size_t pos) const
{
  return Bitboard{std::bitset<size>::operator>>(pos)};
}

template <size_t size>
constexpr Bitboard<size>& Bitboard<size>::operator>>=(size_t pos)
{
  Bitboard{std::bitset<size>::operator>>=(pos)};
  return *this;
}

inline const Bitboard<64> kEmptyBoard = Bitboard<64>{};
