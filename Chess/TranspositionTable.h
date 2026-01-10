#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "Eval.h"
#include "Hasher.h"
#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine {

class Position;

using ShortHash = std::uint16_t;
using Generation = std::uint8_t;

enum class Bound : std::uint8_t {
  kNone = 0,
  kLower = 1,
  kUpper = 2,
  kExact = kLower | kUpper
};

inline std::uint8_t operator&(const Bound lhs, const Bound rhs) {
  return static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs);
}

struct Node {
  Move move{};
  Eval score{};
  Depth depth{};
  Bound bound{Bound::kNone};
  bool is_pv_node{false};

  Node() = default;

  Node(Move m, Eval s, Depth d, Bound b, bool pv)
      : move(m), score(s), depth(d), bound(b), is_pv_node(pv) {}
};

#pragma pack(push, 1)
struct TableEntry {
  ShortHash short_hash{0};
  Depth depth_stored{0};
  Generation generation : 5{};
  Bound bound : 2 {};
  bool is_pv : 1 {};
  Move move{};
  Eval score{0};
  Eval static_eval{0};

  [[nodiscard]] bool IsOccupied() const { return depth_stored != 0; }
  [[nodiscard]] Node Read() const;
  void Save(Hash key, Eval score, bool is_pv_node, Bound bound, Depth depth,
            Move move, Eval static_eval, Generation generation);
  [[nodiscard]] std::uint8_t RelativeAge(Generation current_generation) const;
};
#pragma pack(pop)

struct EntryCluster {
  static constexpr size_t kClusterSize = 3;
  std::array<TableEntry, kClusterSize> entries{};
  std::array<std::byte, 2> padding{};
};

static_assert(sizeof(TableEntry) == 10, "TableEntry must be 10 bytes");
static_assert(sizeof(EntryCluster) == 32,
              "EntryCluster must be 32 bytes for optimal cache performance");

struct ProbeResult {
  bool found;
  Node data;
  std::reference_wrapper<TableEntry> entry;
};

class TranspositionTable {
 public:
  static constexpr size_t kClusterSize = EntryCluster::kClusterSize;
  static constexpr size_t kBytesPerCluster = sizeof(EntryCluster);
  static constexpr size_t kDefaultSizeMB = 640;
  static constexpr size_t kClusterCount =
      (kDefaultSizeMB * 1024 * 1024) / kBytesPerCluster;

  TranspositionTable() { Clear(); }

  void Clear();
  void NewSearch() { generation_ += kGenerationDelta; }
  [[nodiscard]] Generation GetGeneration() const { return generation_; }
  [[nodiscard]] int HashFull(int max_age = 0) const;
  [[nodiscard]] ProbeResult Probe(Hash key);

 private:
  [[nodiscard]] std::array<TableEntry, 3>::iterator GetFirstEntry(Hash key);

  std::vector<EntryCluster> table_;
  Generation generation_{0};

  static constexpr Generation kGenerationDelta = 1;
  static constexpr std::uint8_t kGenerationMask = 0x1F;
};

inline void TranspositionTable::Clear() {
  table_.clear();
  table_.resize(kClusterCount);
  generation_ = 0;
}

inline int TranspositionTable::HashFull(int max_age) const {
  int count = 0;

  const size_t sample_size = std::min<size_t>(1000, kClusterCount);
  for (size_t i = 0; i < sample_size; ++i) {
    for (size_t j = 0; j < kClusterSize; ++j) {
      const auto& entry = table_[i].entries[j];
      if (entry.IsOccupied() && entry.RelativeAge(generation_) <= max_age) {
        ++count;
      }
    }
  }

  return count / kClusterSize;
}

inline ProbeResult TranspositionTable::Probe(Hash key) {
  auto first_entry = GetFirstEntry(key);
  const ShortHash short_hash = static_cast<ShortHash>(key);
  for (size_t i = 0; i < kClusterSize; ++i) {
    if (first_entry[i].short_hash == short_hash) {
      return {first_entry[i].IsOccupied(), first_entry[i].Read(),
              std::ref(first_entry[i])};
    }
  }

  auto replace = first_entry;
  for (size_t i = 1; i < kClusterSize; ++i) {
    const auto replace_value =
        replace->depth_stored - replace->RelativeAge(generation_);
    const auto candidate_value =
        first_entry[i].depth_stored - first_entry[i].RelativeAge(generation_);

    if (replace_value > candidate_value) {
      replace = &first_entry[i];
    }
  }

  return {false, Node{Move::None(), 0, 0, Bound::kNone, false},
          std::ref(*replace)};
}

inline std::array<TableEntry, 3>::iterator TranspositionTable::GetFirstEntry(
    Hash key) {
  const size_t cluster_index = static_cast<size_t>(
      (static_cast<__uint128_t>(key) * kClusterCount) >> 64);
  return table_[cluster_index].entries.begin();
}

inline Node TableEntry::Read() const {
  return Node{move, score, static_cast<Depth>(depth_stored), bound, is_pv};
}

inline void TableEntry::Save(Hash key, Eval score_value, bool is_pv_node,
                             Bound bound_value, Depth depth_value,
                             Move move_value, Eval static_eval_value,
                             Generation gen) {
  const bool should_replace = !IsOccupied() || bound_value == Bound::kExact ||
                              (!(this->bound == Bound::kExact) &&
                               depth_value + 4 > this->depth_stored) ||
                              RelativeAge(gen) > 0;

  if (should_replace) {
    this->short_hash = static_cast<ShortHash>(key);
    this->depth_stored = static_cast<std::uint8_t>(depth_value);
    this->generation = gen;
    this->bound = bound_value;
    this->is_pv = is_pv_node;
    this->score = score_value;
    this->static_eval = static_eval_value;
    this->move = move_value;
  }
}

inline std::uint8_t TableEntry::RelativeAge(
    Generation current_generation) const {
  return (32 + current_generation - generation) & 0x1F;
}

}  // namespace SimpleChessEngine
