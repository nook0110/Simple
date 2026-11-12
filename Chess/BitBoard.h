#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>

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

  constexpr Bitboard &Set(const size_t pos) {
    value_ |= 1ull << pos;
    return *this;
  }

  constexpr Bitboard &Set() {
    value_ = ~0ull;
    return *this;
  }

  constexpr Bitboard &Reset(const size_t pos) {
    value_ &= ~(1ull << pos);
    return *this;
  }

  constexpr Bitboard &Reset() {
    value_ = 0ull;
    return *this;
  }

  constexpr Bitboard &Flip(const size_t pos) {
    value_ ^= 1ull << pos;
    return *this;
  }

  [[nodiscard]] constexpr bool Test(const size_t pos) const {
    return static_cast<bool>(value_ & (1ull << pos));
  }

  constexpr bool operator==(const Bitboard &other) const = default;

  [[nodiscard]] constexpr size_t Count() const;

  [[nodiscard]] constexpr BitIndex GetFirstBit() const;

  constexpr BitIndex PopFirstBit();

  [[nodiscard]] constexpr bool MoreThanOne() const;

  constexpr Bitboard operator-() const;

  constexpr Bitboard operator-(const Bitboard &other) const;
  constexpr Bitboard &operator-=(const Bitboard &other);

  constexpr Bitboard operator*(const Bitboard &other) const;
  constexpr Bitboard &operator*=(const Bitboard &other);

  constexpr Bitboard operator&(const Bitboard &other) const;
  constexpr Bitboard &operator&=(const Bitboard &other);

  constexpr Bitboard operator|(const Bitboard &other) const;
  constexpr Bitboard &operator|=(const Bitboard &other);

  constexpr Bitboard operator^(const Bitboard &other) const;
  constexpr Bitboard &operator^=(const Bitboard &other);

  constexpr Bitboard operator~() const;

  constexpr Bitboard operator<<(size_t pos) const;
  constexpr Bitboard &operator<<=(size_t pos);

  constexpr Bitboard operator>>(size_t pos) const;
  constexpr Bitboard &operator>>=(size_t pos);

  [[nodiscard]] explicit constexpr operator std::uint64_t() const noexcept {
    return value_;
  }

  class Iterator {
   public:
    using value_type = BitIndex;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::forward_iterator_tag;

    constexpr Iterator() = default;
    constexpr explicit Iterator(std::uint64_t remaining) : remaining_(remaining) {}

    [[nodiscard]] constexpr BitIndex operator*() const {
      assert(remaining_ != 0);
      return static_cast<BitIndex>(std::countr_zero(remaining_));
    }

    constexpr Iterator& operator++() {
      assert(remaining_ != 0);
      remaining_ &= remaining_ - 1;
      return *this;
    }

    constexpr Iterator operator++(int) {
      auto previous = *this;
      ++*this;
      return previous;
    }

    constexpr bool operator==(const Iterator&) const = default;

   private:
    std::uint64_t remaining_{};
  };

  [[nodiscard]] constexpr Iterator begin() const { return Iterator{value_}; }
  [[nodiscard]] constexpr Iterator end() const { return Iterator{}; }

 private:
  std::uint64_t value_{};
};

constexpr size_t Bitboard::Count() const {
  return std::popcount(value_);
}

constexpr BitIndex Bitboard::GetFirstBit() const {
  assert(Any());
  return std::countr_zero(value_);
}

constexpr BitIndex Bitboard::PopFirstBit() {
  const BitIndex bit = GetFirstBit();
  value_ &= value_ - 1;
  return bit;
}

constexpr bool Bitboard::MoreThanOne() const {
  return value_ & (value_ - 1); // more effective than std::has_single_bit(value) && Any()
}

constexpr Bitboard Bitboard::operator-() const { return Bitboard{~value_ + 1}; }

constexpr Bitboard Bitboard::operator-(const Bitboard &other) const {
  return Bitboard{value_ - other.value_};  // may (and will) overflow
}

constexpr Bitboard &Bitboard::operator-=(const Bitboard &other) {
  value_ -= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator*(const Bitboard &other) const {
  return Bitboard{value_ * other.value_};  // may (and will) overflow
}

constexpr Bitboard &Bitboard::operator*=(const Bitboard &other) {
  value_ *= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator&(const Bitboard &other) const {
  return Bitboard{value_ & other.value_};
}

constexpr Bitboard &Bitboard::operator&=(const Bitboard &other) {
  value_ &= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator|(const Bitboard &other) const {
  return Bitboard{value_ | other.value_};
}

constexpr Bitboard &Bitboard::operator|=(const Bitboard &other) {
  value_ |= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator^(const Bitboard &other) const {
  return Bitboard{value_ ^ other.value_};
}

constexpr Bitboard &Bitboard::operator^=(const Bitboard &other) {
  value_ ^= other.value_;
  return *this;
}

constexpr Bitboard Bitboard::operator~() const { return Bitboard{~value_}; }

constexpr Bitboard Bitboard::operator<<(const size_t pos) const {
  return Bitboard{value_ << pos};
}

constexpr Bitboard &Bitboard::operator<<=(const size_t pos) {
  value_ <<= pos;
  return *this;
}

constexpr Bitboard Bitboard::operator>>(const size_t pos) const {
  return Bitboard{value_ >> pos};
}

constexpr Bitboard &Bitboard::operator>>=(const size_t pos) {
  value_ >>= pos;
  return *this;
}
