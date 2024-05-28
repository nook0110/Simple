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
#ifdef _DEBUG
    Position position;
#endif
    // sth else...
  };

  [[nodiscard]] bool Contains(const Position& position) const
  {
    return position.GetHash() == GetNode(position).true_hash;
  }

  void SetMove(const Position& position, const Move& move)
  {
    Node inserting_node = {move, position.GetHash()};
#ifdef _DEBUG
    inserting_node.position = position;
#endif

    GetNode(position) = std::move(inserting_node);
  }

  const Move& GetMove(const Position& position) const
  {
    assert(Contains(position));
    return GetNode(position).move;
  }
#ifndef _DEBUG
 private:
#endif

  Node& GetNode(const Position& position)
  {
    return table_[position.GetHash() % TableSize];
  }

  const Node& GetNode(const Position& position) const
  {
    return table_[position.GetHash() % TableSize];
  }

  std::array<Node, TableSize> table_;  //!< The table.
};
}  // namespace SimpleChessEngine
