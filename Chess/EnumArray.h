#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace SimpleChessEngine {

template <class Key>
inline constexpr std::size_t kEnumSize = 0;

template <class Key>
concept IndexEnum = std::is_enum_v<Key> && (kEnumSize<Key> > 0);

template <IndexEnum Key, class Value>
struct EnumArray {
  using value_type = Value;
  using iterator = typename std::array<Value, kEnumSize<Key>>::iterator;
  using const_iterator =
      typename std::array<Value, kEnumSize<Key>>::const_iterator;

  std::array<Value, kEnumSize<Key>> values;

  [[nodiscard]] constexpr Value& operator[](Key key) noexcept {
    return values[std::to_underlying(key)];
  }
  [[nodiscard]] constexpr const Value& operator[](Key key) const noexcept {
    return values[std::to_underlying(key)];
  }

  [[nodiscard]] static constexpr std::size_t size() noexcept {
    return kEnumSize<Key>;
  }

  [[nodiscard]] constexpr iterator begin() noexcept { return values.begin(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return values.begin();
  }
  [[nodiscard]] constexpr iterator end() noexcept { return values.end(); }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return values.end();
  }

  constexpr void fill(const Value& value) { values.fill(value); }

  [[nodiscard]] constexpr bool operator==(const EnumArray&) const = default;
};

}  // namespace SimpleChessEngine
