#pragma once

#include <cstdint>
#include <ostream>
#include <limits>

namespace SimpleChessEngine {

struct Eval {
  std::int16_t value{0};

  constexpr Eval() = default;
  constexpr Eval(std::int16_t v) : value(v) {}
  constexpr Eval(int v) : value(static_cast<std::int16_t>(v)) {}
  constexpr Eval(std::size_t v) : value(static_cast<std::int16_t>(v)) {}

  constexpr auto operator<=>(const Eval&) const = default;
  constexpr bool operator==(const Eval&) const = default;

  constexpr Eval operator-() const { return Eval(-value); }
  constexpr Eval operator+() const { return *this; }

  constexpr Eval operator+(const Eval& other) const {
    return Eval(value + other.value);
  }
  constexpr Eval operator-(const Eval& other) const {
    return Eval(value - other.value);
  }
  constexpr Eval operator*(const std::integral auto scale) const {
    return Eval(value * scale);
  }
  constexpr Eval operator/(const std::integral auto scale) const {
    return Eval(value / scale);
  }

  constexpr Eval& operator+=(const Eval& other) {
    value += other.value;
    return *this;
  }
  constexpr Eval& operator-=(const Eval& other) {
    value -= other.value;
    return *this;
  }
  constexpr Eval& operator*=(const std::integral auto scale) {
    value *= scale;
    return *this;
  }
  constexpr Eval& operator/=(const std::integral auto scale) {
    value /= scale;
    return *this;
  }
};

inline Eval operator*(const std::integral auto scale, const Eval& eval) {
  return eval * scale;
}

inline std::ostream& operator<<(std::ostream& os, const Eval& eval) {
  return os << eval.value;
}

inline constexpr Eval operator""_ev(unsigned long long value) {
    return Eval(static_cast<std::int16_t>(value));
}
}  // namespace SimpleChessEngine

namespace std {
  template<>
  constexpr SimpleChessEngine::Eval numeric_limits<SimpleChessEngine::Eval>::min() noexcept {
    return SimpleChessEngine::Eval(numeric_limits<std::int16_t>::min());
  }
  template<>
  constexpr SimpleChessEngine::Eval numeric_limits<SimpleChessEngine::Eval>::max() noexcept{
    return SimpleChessEngine::Eval(numeric_limits<std::int16_t>::max());
  }
  constexpr SimpleChessEngine::Eval abs(const SimpleChessEngine::Eval eval) noexcept {
    return SimpleChessEngine::Eval(std::abs(eval.value));
  }
}