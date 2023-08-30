#pragma once
#include <array>
#include <optional>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"
namespace SimpleChessEngine
{
class TranspositionTable
{
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
    return table_[position.GetHash() & (kSize - 1)];
  }

  const Node& operator[](const Position& position) const
  {
    return table_[position.GetHash() & (kSize - 1)];
  }

 private:
  static constexpr size_t kSize = 1 << 15;  //!< Size of the table.
  std::array<Node, kSize> table_;           //!< The table.
};
}  // namespace SimpleChessEngine
