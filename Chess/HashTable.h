#pragma once
#include <array>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine
{
template <size_t Size>
class HashTable
{
 public:
  [[nodiscard]] bool Contains(const Position& position) const { return false; }

  Move operator[](const Position& position) const
  {
    return table_[position.GetHash() % Size];
  }

 private:
  std::array<Move, Size> table_;
};
}  // namespace SimpleChessEngine
