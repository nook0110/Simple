#pragma once
#include <algorithm>
#include <array>
#include <random>

#include "Utility.h"

namespace SimpleChessEngine {
class Position;

using Hash = unsigned long long;

struct Hasher {
  template <class RNG>
  explicit Hasher(RNG generator) {
    auto generator_reference = std::reference_wrapper(generator);
    for (auto& piece_type : psqt_hash) {
      for (auto& colored_board : piece_type) {
        std::generate(colored_board.begin(), colored_board.end(),
                      generator_reference);
      }
    }
    std::generate(en_croissant_hash.begin(), en_croissant_hash.end(),
                  generator_reference);
    for (auto& color : cr_hash) {
      std::generate(color.begin(), color.end(), generator_reference);
    }
    stm_hash = generator_reference();
  }

  bool operator==(const Hasher&) const = default;

  std::array<std::array<std::array<Hash, kBoardArea>, kColors>, kPieceTypes>
      psqt_hash{};
  std::array<Hash, kLineSize> en_croissant_hash{};
  std::array<std::array<Hash, 4>, kColors> cr_hash{};
  Hash stm_hash;
};
}  // namespace SimpleChessEngine
