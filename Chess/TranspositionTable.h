#pragma once
#include <array>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"
namespace SimpleChessEngine
{
template <size_t TableSize>
class TranspositionTable
{
  static_assert(!(TableSize & (TableSize - 1)));
 public:
  struct Node
  {
    Move move;
    Hash true_hash{};
    // sth else...
  };

  [[nodiscard]] bool Contains(const Position& position) const
  {
    return position.GetHash() == operator[](position).true_hash;
  }

  Node& operator[](const Position& position)
  {
    return table_[position.GetHash() % TableSize];
  }

  const Node& operator[](const Position& position) const
  {
    return table_[position.GetHash() % TableSize];
  }

 private:
  std::array<Node, TableSize> table_;           //!< The table.
};
}  // namespace SimpleChessEngine
