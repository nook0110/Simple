#pragma once
#include <array>
#include <bit>
#include <vector>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"
namespace SimpleChessEngine {
enum class Bound : std::uint8_t {
  kLower = 1,
  kUpper = 2,
  kExact = kLower | kUpper
};

inline std::uint8_t operator&(const Bound lhs, const Bound rhs) {
  return static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs);
}

#pragma pack(push, 1)
struct Node {
  Hash true_hash{};
  Move move{};
  Eval score{};
  Depth depth : 6 {};
  Bound bound : 2 {};
  Age age{};
};
#pragma pack(pop)

template <size_t TableSize>
  requires(std::has_single_bit(TableSize))
class TranspositionTable {
 public:
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

  std::vector<Node> table_ = std::vector<Node>(TableSize);  //!< The table.
};
}  // namespace SimpleChessEngine
