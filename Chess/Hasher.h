#pragma once

#include <algorithm>

namespace SimpleChessEngine
{
class Position;

using Hash = uint64_t;

struct Hasher
{
  template <class RNG>
  explicit Hasher(RNG generator)
  {
    for (auto& board : psqt_hash)
    {
      std::generate(board.begin(), board.end(), generator);
    }
    std::generate(en_croissant_hash.begin(), en_croissant_hash.end(),
                  generator);
    stm_hash = generator();
  }

  bool operator==(const Hasher&) const = default;

  std::array<std::array<Hash, kBoardArea>, kPieceTypes> psqt_hash{};
  std::array<Hash, kLineSize> en_croissant_hash{};
  Hash stm_hash;
};
}  // namespace SimpleChessEngine
