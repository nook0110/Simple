#pragma once
#include <algorithm>
#include <array>

#include "Utility.h"

namespace SimpleChessEngine
{
class Position;

using Hash = unsigned long long;

struct Hasher
{
  template <class RNG>
  explicit Hasher(RNG generator)

  {
    auto generator_reference = std::reference_wrapper(generator);
    for (auto& board : psqt_hash)
    {
      std::generate(board.begin(), board.end(), generator_reference);
    }
    std::generate(en_croissant_hash.begin(), en_croissant_hash.end(),
                  generator_reference);
    for (auto& color : cr_hash)
    {
      std::generate(color.begin(), color.end(), generator_reference);
    }
    stm_hash = generator_reference();
  }

  bool operator==(const Hasher&) const = default;

  std::array<std::array<Hash, kBoardArea>, kPieceTypes> psqt_hash{};
  std::array<Hash, kLineSize> en_croissant_hash{};
  std::array<std::array<Hash, 4>, kColors> cr_hash{};
  Hash stm_hash;
};
}  // namespace SimpleChessEngine
