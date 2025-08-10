#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>

using BitIndex = std::int8_t;

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

  constexpr explicit Bitboard(const std::uint64_t value) : value_(value) {}

  constexpr Bitboard() = default;

  [[nodiscard]] constexpr bool Any() const { return static_cast<bool>(value_); }
  [[nodiscard]] constexpr bool None() const {
    return !static_cast<bool>(value_);
  }

  Bitboard &Set(const size_t pos) {
    value_ |= 1ull << pos;
    return *this;
  }

  Bitboard &Set() {
    value_ = ~0ull;
    return *this;
  }

  Bitboard &Reset(const size_t pos) {
    value_ &= ~(1ull << pos);
    return *this;
  }

  Bitboard &Reset() {
    value_ = 0ull;
    return *this;
  }

  Bitboard &Flip(const size_t pos) {
    value_ ^= 1ull << pos;
    return *this;
  }

  [[nodiscard]] constexpr bool Test(const size_t pos) const {
    return static_cast<bool>(value_ & (1ull << pos));
  }

  constexpr bool operator==(const Bitboard &other) const = default;

  [[nodiscard]] size_t Count() const;

  [[nodiscard]] BitIndex GetFirstBit() const;

  BitIndex PopFirstBit();

  [[nodiscard]] bool MoreThanOne() const;

  constexpr Bitboard operator-() const;

  constexpr Bitboard operator-(const Bitboard &other) const;
  Bitboard &operator-=(const Bitboard &other);

  constexpr Bitboard operator*(const Bitboard &other) const;
  Bitboard &operator*=(const Bitboard &other);

  constexpr Bitboard operator&(const Bitboard &other) const;
  Bitboard &operator&=(const Bitboard &other);

  constexpr Bitboard operator|(const Bitboard &other) const;
  Bitboard &operator|=(const Bitboard &other);

  constexpr Bitboard operator^(const Bitboard &other) const;
  Bitboard &operator^=(const Bitboard &other);

  constexpr Bitboard operator~() const;

  constexpr Bitboard operator<<(size_t pos) const;
  Bitboard &operator<<=(size_t pos);

  constexpr Bitboard operator>>(size_t pos) const;
  Bitboard &operator>>=(size_t pos);

  [[nodiscard]] explicit constexpr operator std::uint64_t() const noexcept {
    return value_;
  }

 private:
  std::uint64_t value_{};
};

inline size_t Bitboard::Count() const {
  return std::popcount(value_);
}

inline BitIndex Bitboard::GetFirstBit() const {
  assert(Any());
  return std::countr_zero(value_);
}

inline BitIndex Bitboard::PopFirstBit() {
  const BitIndex bit = GetFirstBit();
  Reset(bit);
  return bit;
}

inline bool Bitboard::MoreThanOne() const {
  return value_ & (value_ - 1); // more effective than std::has_single_bit(value) && Any()
}

constexpr Bitboard Bitboard::operator-() const { return Bitboard{~value_ + 1}; }

constexpr Bitboard Bitboard::operator-(const Bitboard &other) const {
  return Bitboard{value_ - other.value_};  // may (and will) overflow
}

inline Bitboard &Bitboard::operator-=(const Bitboard &other) {
  value_ -= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator*(const Bitboard &other) const {
  return Bitboard{value_ * other.value_};  // may (and will) overflow
}

inline Bitboard &Bitboard::operator*=(const Bitboard &other) {
  value_ *= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator&(const Bitboard &other) const {
  return Bitboard{value_ & other.value_};
}

inline Bitboard &Bitboard::operator&=(const Bitboard &other) {
  value_ &= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator|(const Bitboard &other) const {
  return Bitboard{value_ | other.value_};
}

inline Bitboard &Bitboard::operator|=(const Bitboard &other) {
  value_ |= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator^(const Bitboard &other) const {
  return Bitboard{value_ ^ other.value_};
}

inline Bitboard &Bitboard::operator^=(const Bitboard &other) {
  value_ ^= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator~() const { return Bitboard{~value_}; }

constexpr Bitboard Bitboard::operator<<(const size_t pos) const {
  return Bitboard{value_ << pos};
}

inline Bitboard &Bitboard::operator<<=(const size_t pos) {
  value_ <<= pos;
  return *this;
}

constexpr Bitboard Bitboard::operator>>(const size_t pos) const {
  return Bitboard{value_ >> pos};
}

inline Bitboard &Bitboard::operator>>=(const size_t pos) {
  value_ >>= pos;
  return *this;
}
