#pragma once
#include <optional>

#include "BitScan.h"

constexpr size_t kBitboardSize = 64;

/**
 * \brief Class that represents a bitboard.
 *
 * \details A simple wrapper of std::bitset.
 *
 * \tparam size The size of the bitboard.
 *
 * \author nook0110
 */
class Bitboard : public std::bitset<kBitboardSize>
{
  // TODO: Add constexpr to all methods

 public:
  using std::bitset<kBitboardSize>::bitset;

  /**
   * \brief Constructs a bitboard from a bitset.
   */

  explicit Bitboard(std::bitset<kBitboardSize> bitset)
      : std::bitset<kBitboardSize>(std::move(bitset))
  {}

  [[nodiscard]] std::optional<BitIndex> GetFirstBit() const;

  Bitboard operator&(const Bitboard& other) const;
  Bitboard& operator&=(const Bitboard& other);

  Bitboard operator|(const Bitboard& other) const;
  Bitboard& operator|=(const Bitboard& other);

  Bitboard operator^(const Bitboard& other) const;
  Bitboard& operator^=(const Bitboard& other);

  Bitboard operator~() const;

  Bitboard operator<<(size_t pos) const;
  Bitboard& operator<<=(size_t pos);

  Bitboard operator>>(size_t pos) const;
  Bitboard& operator>>=(size_t pos);

 private:
};

inline std::optional<BitIndex> Bitboard::GetFirstBit() const
{
  return BitScan(this->to_ullong());
}

inline Bitboard Bitboard::operator&(const Bitboard& other) const
{
  auto copy = *this;
  return copy &= other;
}

inline Bitboard& Bitboard::operator&=(const Bitboard& other)
{
  std::bitset<kBitboardSize>::operator&=(other);
  return *this;
}

inline Bitboard Bitboard::operator|(const Bitboard& other) const
{
  auto copy = *this;
  return copy |= other;
}

inline Bitboard& Bitboard::operator|=(const Bitboard& other)
{
  std::bitset<kBitboardSize>::operator|=(other);
  return *this;
}

inline Bitboard Bitboard::operator^(const Bitboard& other) const
{
  auto copy = *this;
  return copy ^= other;
}

inline Bitboard& Bitboard::operator^=(const Bitboard& other)
{
  Bitboard{std::bitset<kBitboardSize>::operator^=(other)};
  return *this;
}

inline Bitboard Bitboard::operator~() const
{
  return Bitboard{std::bitset<kBitboardSize>::operator~()};
}

inline Bitboard Bitboard::operator<<(const size_t pos) const
{
  return Bitboard{std::bitset<kBitboardSize>::operator<<(pos)};
}

inline Bitboard& Bitboard::operator<<=(const size_t pos)
{
  Bitboard{std::bitset<kBitboardSize>::operator<<=(pos)};
  return *this;
}

inline Bitboard Bitboard::operator>>(const size_t pos) const
{
  return Bitboard{std::bitset<kBitboardSize>::operator>>(pos)};
}

inline Bitboard& Bitboard::operator>>=(const size_t pos)
{
  Bitboard{std::bitset<kBitboardSize>::operator>>=(pos)};
  return *this;
}
