#pragma once
#include <array>

#include "Utility.h"

namespace SimpleChessEngine {
class Position;

using Hash = std::uint64_t;

class Mt19937_64 {
 public:
  constexpr explicit Mt19937_64(Hash seed) {
    state_[0] = seed;
    for (size_t i = 1; i < state_.size(); ++i) {
      const auto previous = state_[i - 1];
      state_[i] = 6364136223846793005ull * (previous ^ (previous >> 62)) + i;
    }
  }

  [[nodiscard]] constexpr Hash operator()() {
    if (index_ == state_.size()) Twist();
    auto value = state_[index_++];
    value ^= (value >> 29) & 0x5555555555555555ull;
    value ^= (value << 17) & 0x71d67fffeda60000ull;
    value ^= (value << 37) & 0xfff7eee000000000ull;
    return value ^ (value >> 43);
  }

 private:
  constexpr void Twist() {
    constexpr Hash kLowerMask = (Hash{1} << 31) - 1;
    constexpr Hash kUpperMask = ~kLowerMask;
    for (size_t i = 0; i < state_.size(); ++i) {
      const auto joined = (state_[i] & kUpperMask) |
                          (state_[(i + 1) % state_.size()] & kLowerMask);
      state_[i] = state_[(i + kRecurrenceOffset) % state_.size()] ^
                  (joined >> 1) ^ ((joined & 1) ? 0xb5026f5aa96619e9ull : 0);
    }
    index_ = 0;
  }

  static constexpr size_t kStateSize = 312;
  static constexpr size_t kRecurrenceOffset = 156;
  std::array<Hash, kStateSize> state_{};
  size_t index_ = state_.size();
};

struct Hasher {
  template <class RNG>
  explicit constexpr Hasher(RNG generator) {
    const auto fill = [&generator](auto& values) {
      for (auto& value : values) value = generator();
    };
    for (auto& piece_type : psqt_hash) {
      for (auto& colored_board : piece_type) {
        fill(colored_board);
      }
    }
    fill(en_croissant_hash);
    for (auto& color : cr_hash) {
      fill(color);
    }
    stm_hash = generator();
  }

  constexpr bool operator==(const Hasher&) const = default;

  std::array<std::array<std::array<Hash, kBoardArea>, kColors>, kPieceTypes>
      psqt_hash{};
  std::array<Hash, kLineSize> en_croissant_hash{};
  std::array<std::array<Hash, 4>, kColors> cr_hash{};
  Hash stm_hash;
};
extern const Hasher kZobristKeys;

}  // namespace SimpleChessEngine
