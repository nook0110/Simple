#pragma once

#ifdef __GNUC__
#define USE_GCC_BUILTINS
#pragma GCC target("avx2,bmi,bmi2,popcnt,lzcnt")
#elif defined(_MSC_VER)
#define USE_MSVC_INTRINSICS
#include "nmmintrin.h"
#endif

#include <optional>

#include "BitScan.h"

constexpr size_t kBitboardSize = 64;

/**
 * \brief Class that represents a bitboard.
 *
 * \details A simple wrapper of 64-bit integer.
 *
 * \tparam size The size of the bitboard.
 *
 * \author nook0110
 * \author alfoos
 */
class Bitboard
{
 public:
  /**
   * \brief Constructs a bitboard from its value.
   */

  constexpr explicit Bitboard(const uint64_t value) : value_(value) {}

  constexpr Bitboard() {}

  [[nodiscard]] constexpr bool any() const { return static_cast<bool>(value_); }
  [[nodiscard]] constexpr bool none() const
  {
    return !static_cast<bool>(value_);
  }

  Bitboard& set(const size_t pos)
  {
    value_ |= 1ull << pos;
    return *this;
  }

  Bitboard& set()
  {
    value_ = ~0ull;
    return *this;
  }

  Bitboard& reset(const size_t pos)
  {
    value_ &= ~(1ull << pos);
    return *this;
  }

  Bitboard& reset()
  {
    value_ = 0ull;
    return *this;
  }

  Bitboard& flip(const size_t pos)
  {
    value_ ^= 1ull << pos;
    return *this;
  }

  constexpr bool test(const size_t pos) const
  {
    return static_cast<bool>(value_ & (1ull << pos));
  }

  constexpr bool operator==(const Bitboard& other) const = default;

  size_t count() const;

  [[nodiscard]] std::optional<BitIndex> GetFirstBit() const;

  constexpr Bitboard operator-(const Bitboard& other) const;
  Bitboard& operator-=(const Bitboard& other);

  constexpr Bitboard operator*(const Bitboard& other) const;
  Bitboard& operator*=(const Bitboard& other);

  constexpr Bitboard operator&(const Bitboard& other) const;
  Bitboard& operator&=(const Bitboard& other);

  constexpr Bitboard operator|(const Bitboard& other) const;
  Bitboard& operator|=(const Bitboard& other);

  constexpr Bitboard operator^(const Bitboard& other) const;
  Bitboard& operator^=(const Bitboard& other);

  constexpr Bitboard operator~() const;

  constexpr Bitboard operator<<(size_t pos) const;
  Bitboard& operator<<=(size_t pos);

  constexpr Bitboard operator>>(size_t pos) const;
  Bitboard& operator>>=(size_t pos);

  [[nodiscard]] explicit constexpr operator unsigned long long() const noexcept
  {
    return value_;
  }

 private:
  uint64_t value_{};
};

inline size_t Bitboard::count() const
{
#ifdef USE_GCC_BUILTINS
  return __builtin_popcountll(value_);
#elif defined(USE_MSVC_INTRINSICS)
  return _mm_popcnt_u64(value_);
#endif
  return std::bitset<64>(value_).count();
}

inline std::optional<BitIndex> Bitboard::GetFirstBit() const
{
  return BitScan(value_);
}

constexpr Bitboard Bitboard::operator-(const Bitboard& other) const
{
  return Bitboard{value_ - other.value_};  // may (and will) overflow
}

inline Bitboard& Bitboard::operator-=(const Bitboard& other)
{
  value_ -= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator*(const Bitboard& other) const
{
  return Bitboard{value_ * other.value_};  // may (and will) overflow
}

inline Bitboard& Bitboard::operator*=(const Bitboard& other)
{
  value_ *= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator&(const Bitboard& other) const
{
  return Bitboard{value_ & other.value_};
}

inline Bitboard& Bitboard::operator&=(const Bitboard& other)
{
  value_ &= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator|(const Bitboard& other) const
{
  return Bitboard{value_ | other.value_};
}

inline Bitboard& Bitboard::operator|=(const Bitboard& other)
{
  value_ |= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator^(const Bitboard& other) const
{
  return Bitboard{value_ ^ other.value_};
}

inline Bitboard& Bitboard::operator^=(const Bitboard& other)
{
  value_ ^= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator~() const { return Bitboard{~value_}; }

constexpr Bitboard Bitboard::operator<<(const size_t pos) const
{
  return Bitboard{value_ << pos};
}

inline Bitboard& Bitboard::operator<<=(const size_t pos)
{
  value_ <<= pos;
  return *this;
}

constexpr Bitboard Bitboard::operator>>(const size_t pos) const
{
  return Bitboard{value_ >> pos};
}

inline Bitboard& Bitboard::operator>>=(const size_t pos)
{
  value_ >>= pos;
  return *this;
}

#undef USE_GCC_BUILTINS
#undef USE_MSVC_INTRINSICS
