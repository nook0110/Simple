#pragma once
#include <bit>
#include <cassert>
#include <cstddef>
#include <vector>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"
namespace SimpleChessEngine {
enum class Bound : std::uint8_t {
  kUpper = 1,
  kLower = 2,
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

  auto operator<=>(const Node& rhs) const {
    return std::forward_as_tuple(bound, depth + age * 2) <=>
           std::forward_as_tuple(rhs.bound, rhs.depth + rhs.age * 2);
  };

  bool operator==(const Node& rhs) const = default;
};
#pragma pack(pop)

class Bucket {
 public:
  constexpr static size_t kBucketSize = 2;

  Bucket() { bucket_.reserve(kBucketSize); }

  bool Contains(const Position& position) const {
    return std::ranges::any_of(bucket_, [&](const Node& node) {
      return node.true_hash == position.GetHash();
    });
  }

  std::optional<Node> GetNode(const Position& position) const {
    auto it = std::ranges::find_if(bucket_, [&](const Node& node) {
      return node.true_hash == position.GetHash();
    });
    return it != bucket_.end() ? std::make_optional(*it) : std::nullopt;
  }

  void SetEntry(const Position& position, const Move& move, const Eval score,
                const Depth depth, const Bound bound, const Age age) {
    Node inserting_node = {position.GetHash(), move, score, depth, bound, age};
    if (bucket_.size() < kBucketSize) {
      bucket_.push_back(inserting_node);
      return;
    }
    auto& worst_node = *std::ranges::min_element(bucket_);
    if (inserting_node > worst_node) {
      worst_node = inserting_node;
    }
  }

 private:
  std::vector<Node> bucket_;
};

template <size_t TableSize>
  requires(std::has_single_bit(TableSize))
class TranspositionTable {
 public:
  [[nodiscard]] bool Contains(const Position& position) const {
    return GetBucket(position).Contains(position);
  }

  void SetEntry(const Position& position, const Move& move, const Eval score,
                const Depth depth, const Bound bound, const Age age) {
    GetBucket(position).SetEntry(position, move, score, depth, bound, age);
  }

  const Move& GetMove(const Position& position) const {
    assert(Contains(position));
    return GetBucket(position).GetNode(position).move;
  }

  std::optional<Node> GetNode(const Position& position) const {
    return GetBucket(position).GetNode(position);
  }

 private:
  const Bucket& GetBucket(const Position& position) const {
    return table_[position.GetHash() % TableSize];
  }

  Bucket& GetBucket(const Position& position) {
    return table_[position.GetHash() % TableSize];
  }

  std::vector<Bucket> table_ = std::vector<Bucket>(TableSize);  //!< The table.
};
}  // namespace SimpleChessEngine
