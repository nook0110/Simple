#pragma once
#include <array>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"
namespace SimpleChessEngine {
enum class Bound : uint8_t { kLower = 1, kUpper = 2, kExact = kLower | kUpper };

inline uint8_t operator&(const Bound lhs, const Bound rhs) {
  return static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs);
}

template <size_t TableSize>
class TranspositionTable {
  static_assert(!(TableSize & (TableSize - 1)));

 public:
#pragma pack(push, 1)
  struct Node {
    Hash true_hash{};
    Move move;
    Eval score;
    Depth depth : 6;
    Bound bound : 2;
    Age age;
  };
#pragma pack(pop)

  [[nodiscard]] bool Contains(const Position& position) const {
    return position.GetHash() == GetNode(position).true_hash;
  }

  void SetEntry(const Position& position, const Move& move, const Eval score,
                const Depth depth, const Bound bound, const Age age) {
    Node inserting_node = {position.GetHash(), move, score, depth, bound, age};
    auto& entry_node = GetNode(position);
    if (bound == Bound::kExact ||
        !(entry_node.bound == Bound::kExact && entry_node.age == age)) {
      entry_node = inserting_node;
    }
  }

  const Move& GetMove(const Position& position) const {
    assert(Contains(position));
    return GetNode(position).move;
  }

  Node& GetNode(const Position& position) {
    return table_[position.GetHash() % TableSize];
  }

  const Node& GetNode(const Position& position) const {
    return table_[position.GetHash() % TableSize];
  }

  std::array<Node, TableSize> table_;  //!< The table.
};
}  // namespace SimpleChessEngine
