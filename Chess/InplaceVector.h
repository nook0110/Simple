#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace SimpleChessEngine {

template <std::size_t capacity>
using SmallestSizeType = std::conditional_t<
    capacity <= std::numeric_limits<std::uint8_t>::max(), std::uint8_t,
    std::conditional_t<capacity <= std::numeric_limits<std::uint16_t>::max(),
                       std::uint16_t, std::size_t>>;

template <class T, std::size_t N>
  requires std::is_trivially_copyable_v<T> &&
           std::is_trivially_default_constructible_v<T>
class InplaceVector {
 public:
  using value_type = T;
  using size_type = SmallestSizeType<N>;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = T*;
  using const_iterator = const T*;

  constexpr InplaceVector() noexcept = default;

  [[nodiscard]] static constexpr std::size_t capacity() noexcept { return N; }
  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  [[nodiscard]] constexpr iterator begin() noexcept { return elements_.data(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return elements_.data();
  }
  [[nodiscard]] constexpr iterator end() noexcept { return begin() + size_; }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return begin() + size_;
  }

  [[nodiscard]] constexpr reference operator[](size_type index) noexcept {
    assert(index < size_);
    return elements_[index];
  }
  [[nodiscard]] constexpr const_reference operator[](
      size_type index) const noexcept {
    assert(index < size_);
    return elements_[index];
  }
  [[nodiscard]] constexpr reference front() noexcept { return (*this)[0]; }
  [[nodiscard]] constexpr const_reference front() const noexcept {
    return (*this)[0];
  }

  constexpr reference unchecked_push_back(const T& value) noexcept {
    assert(size_ < N);
    return elements_[size_++] = value;
  }

  template <class Predicate>
  friend constexpr size_type erase_if(InplaceVector& vector,
                                      Predicate predicate) {
    const auto kept_end =
        std::remove_if(vector.begin(), vector.end(), predicate);
    const auto erased = static_cast<size_type>(vector.end() - kept_end);
    vector.size_ = static_cast<size_type>(kept_end - vector.begin());
    return erased;
  }

 private:
  std::array<T, N> elements_;
  size_type size_ = 0;
};

}  // namespace SimpleChessEngine
