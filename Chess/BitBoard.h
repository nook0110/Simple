#pragma once

#include "BitScan.h"

#ifdef __GNUC__
#define USE_GCC_BUILTINS
#pragma GCC target("avx2,bmi,bmi2,popcnt,lzcnt")
#elif defined(_MSC_VER)
#define USE_MSVC_INTRINSICS

#include "nmmintrin.h"
#pragma intrinsic(_BitScanForward)
#endif

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
class Bitboard {
 public:
  /**
   * \brief Constructs a bitboard from its value.
   */

  constexpr explicit Bitboard(const unsigned long long value) : value_(value) {}

  constexpr Bitboard() = default;

  [[nodiscard]] constexpr bool Any() const { return static_cast<bool>(value_); }
  [[nodiscard]] constexpr bool None() const {
    return !static_cast<bool>(value_);
  }

  Bitboard& Set(const size_t pos) {
    value_ |= 1ull << pos;
    return *this;
  }

  Bitboard& Set() {
    value_ = ~0ull;
    return *this;
  }

  Bitboard& Reset(const size_t pos) {
    value_ &= ~(1ull << pos);
    return *this;
  }

  Bitboard& Reset() {
    value_ = 0ull;
    return *this;
  }

  Bitboard& Flip(const size_t pos) {
    value_ ^= 1ull << pos;
    return *this;
  }

  [[nodiscard]] constexpr bool Test(const size_t pos) const {
    return static_cast<bool>(value_ & 1ull << pos);
  }

  constexpr bool operator==(const Bitboard& other) const = default;

  [[nodiscard]] size_t Count() const;

  [[nodiscard]] BitIndex GetFirstBit() const;

  BitIndex PopFirstBit();

  [[nodiscard]] bool MoreThanOne() const;

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

  [[nodiscard]] explicit constexpr operator unsigned long long()
      const noexcept {
    return value_;
  }

 private:
  unsigned long long value_{};
};

inline size_t Bitboard::Count() const {
#ifdef USE_GCC_BUILTINS
  return __builtin_popcountll(value_);
#elif defined(USE_MSVC_INTRINSICS)
  return _mm_popcnt_u64(value_);
#endif
}

inline BitIndex Bitboard::GetFirstBit() const {
  assert(Any());
  return BitScan(value_);
}

inline BitIndex Bitboard::PopFirstBit() {
  const BitIndex bit = GetFirstBit();
  Reset(bit);
  return bit;
}

inline bool Bitboard::MoreThanOne() const {
  return (*this & (*this - Bitboard{1})).Any();
}

constexpr Bitboard Bitboard::operator-(const Bitboard& other) const {
  return Bitboard{value_ - other.value_};  // may (and will) overflow
}

inline Bitboard& Bitboard::operator-=(const Bitboard& other) {
  value_ -= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator*(const Bitboard& other) const {
  return Bitboard{value_ * other.value_};  // may (and will) overflow
}

inline Bitboard& Bitboard::operator*=(const Bitboard& other) {
  value_ *= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator&(const Bitboard& other) const {
  return Bitboard{value_ & other.value_};
}

inline Bitboard& Bitboard::operator&=(const Bitboard& other) {
  value_ &= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator|(const Bitboard& other) const {
  return Bitboard{value_ | other.value_};
}

inline Bitboard& Bitboard::operator|=(const Bitboard& other) {
  value_ |= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator^(const Bitboard& other) const {
  return Bitboard{value_ ^ other.value_};
}

inline Bitboard& Bitboard::operator^=(const Bitboard& other) {
  value_ ^= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator~() const { return Bitboard{~value_}; }

constexpr Bitboard Bitboard::operator<<(const size_t pos) const {
  return Bitboard{value_ << pos};
}

inline Bitboard& Bitboard::operator<<=(const size_t pos) {
  value_ <<= pos;
  return *this;
}

constexpr Bitboard Bitboard::operator>>(const size_t pos) const {
  return Bitboard{value_ >> pos};
}

inline Bitboard& Bitboard::operator>>=(const size_t pos) {
  value_ >>= pos;
  return *this;
}

#undef USE_GCC_BUILTINS
#undef USE_MSVC_INTRINSICS
