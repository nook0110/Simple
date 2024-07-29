#pragma once

#include <array>

template <class UnderlyingType, size_t size, class EnumType>
  requires(std::is_enum_v<EnumType>)
class EnumArray : public std::array<UnderlyingType, size> {
 public:
  std::array<UnderlyingType, size>::reference operator[](EnumType pos) {
    return std::array<UnderlyingType, size>::operator[](
        static_cast<size_t>(pos));
  }

  std::array<UnderlyingType, size>::const_reference operator[](
      EnumType pos) const {
    return std::array<UnderlyingType, size>::operator[](
        static_cast<size_t>(pos));
  }
};
