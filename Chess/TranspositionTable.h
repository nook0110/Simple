#pragma once

#include <array>
#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <new>
#include <sys/mman.h>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine {
inline constexpr Eval kNoStaticEval = -8192;

inline Eval TTScorePlyAdjustment(const Eval value, const Depth ply) {
  return std::abs(value) >= kTablebaseWinValue - kMaxSearchPly
             ? ((value > 0) - (value < 0)) * ply
             : 0;
}

inline Eval ToTTScore(const Eval value, const Depth ply) {
  return value + TTScorePlyAdjustment(value, ply);
}

inline Eval FromTTScore(const Eval value, const Depth ply) {
  return value - TTScorePlyAdjustment(value, ply);
}

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
  Hash true_hash{};
  Move move{Move::None()};
  Eval score{};
  Depth depth{};
  Bound bound{Bound::kNone};
  std::uint8_t age{};
  Eval static_eval{kNoStaticEval};

  [[nodiscard]] bool IsOccupied() const { return bound != Bound::kNone; }
};

template <size_t TableSizeMB>
class TranspositionTable {
 private:
  struct AtomicEntry {
    mutable std::uint64_t verification;
    mutable std::uint64_t payload;

    [[nodiscard]] Node Load() const {
      const auto stored_verification =
          std::atomic_ref{verification}.load(std::memory_order_acquire);
      const auto stored_payload =
          std::atomic_ref{payload}.load(std::memory_order_relaxed);
      if (stored_payload == 0) return {};
      return Unpack(stored_verification ^ stored_payload, stored_payload);
    }

    void Store(const Node& node) {
      const auto packed = Pack(node);
      std::atomic_ref{payload}.store(packed, std::memory_order_relaxed);
      std::atomic_ref{verification}.store(node.true_hash ^ packed,
                                          std::memory_order_release);
    }
  };

  struct alignas(64) Cluster {
    std::array<AtomicEntry, 4> entries;
  };

 public:
  static constexpr size_t kClusterSize = 4;
  static constexpr size_t kMemoryBytes = TableSizeMB * 1024ULL * 1024ULL;
  static constexpr size_t kClusterCount = kMemoryBytes / sizeof(Cluster);
  static constexpr size_t kEntryCount = kClusterCount * kClusterSize;

  static_assert(TableSizeMB > 0);
  static_assert(sizeof(AtomicEntry) == 16);
  static_assert(sizeof(Cluster) == 64);
  static_assert(std::atomic_ref<std::uint64_t>::is_always_lock_free);
  static_assert(kMemoryBytes % sizeof(Cluster) == 0);

  TranspositionTable() : table_(Allocate()) {}

  [[nodiscard]] bool Contains(const Position& position) const {
    const Node node = GetNode(position);
    return node.IsOccupied() && node.true_hash == position.GetHash();
  }

  void SetEntry(const Position& position, const Move& move, const Eval score,
                const Depth depth, const Bound bound, const Age age,
                const Eval static_eval = kNoStaticEval,
                const std::uint64_t eval_context = 0) {
    const auto hash = position.GetHash();
    auto& cluster = table_[Index(hash)];
    const auto incoming_age = static_cast<std::uint8_t>(age);

    AtomicEntry* replacement = &cluster.entries.front();
    Node replacement_node = replacement->Load();

    for (auto& entry : cluster.entries) {
      const Node current = entry.Load();
      if (current.true_hash == hash) {
        replacement = &entry;
        replacement_node = current;
        break;
      }
      if (!current.IsOccupied()) {
        replacement = &entry;
        replacement_node = current;
        break;
      }
      if (ReplacementValue(current, incoming_age) <
          ReplacementValue(replacement_node, incoming_age)) {
        replacement = &entry;
        replacement_node = current;
      }
    }

    if (!replacement_node.IsOccupied() || replacement_node.true_hash != hash ||
        bound == Bound::kExact || replacement_node.age != incoming_age ||
        (replacement_node.bound != Bound::kExact &&
         static_cast<unsigned>(depth) + 4 > replacement_node.depth)) {
      Eval cached_eval = kNoStaticEval;
      if (static_eval > kNoStaticEval && static_eval < -kNoStaticEval) {
        auto context = eval_context_.load(std::memory_order_relaxed);
        if (context == 0) {
          eval_context_.compare_exchange_strong(
              context, eval_context, std::memory_order_relaxed);
          context = eval_context_.load(std::memory_order_relaxed);
        }
        if (context == eval_context) cached_eval = static_eval;
      }
      replacement->Store(
          Node{hash,
               !move.IsValid() && replacement_node.true_hash == hash
                   ? replacement_node.move
                   : move,
               score, depth, bound, incoming_age, cached_eval});
    }
  }

  [[nodiscard]] Move GetMove(const Position& position) const {
    const Node node = GetNode(position);
    assert(node.true_hash == position.GetHash());
    return node.move;
  }

  [[nodiscard]] Node GetNode(const Position& position) const {
    const auto hash = position.GetHash();
    const auto& cluster = table_[Index(hash)];
    for (const auto& entry : cluster.entries) {
      const Node node = entry.Load();
      if (node.IsOccupied() && node.true_hash == hash) return node;
    }
    return {};
  }

  [[nodiscard]] Eval GetStaticEval(const Node& node,
                                   const std::uint64_t eval_context) const {
    return eval_context_.load(std::memory_order_relaxed) == eval_context
               ? node.static_eval
               : kNoStaticEval;
  }

  static constexpr std::uint64_t Pack(const Node& node) {
    assert(node.score >= -(1 << 17) && node.score < (1 << 17));
    const auto score = static_cast<std::uint32_t>(node.score) & 0x3FFFF;
    const auto static_eval =
        static_cast<std::uint32_t>(
            node.static_eval > kNoStaticEval && node.static_eval < -kNoStaticEval
                ? node.static_eval
                : kNoStaticEval) &
        0x3FFF;
    return static_cast<std::uint64_t>(node.move.Raw()) |
           (static_cast<std::uint64_t>(score) << 16) |
           (static_cast<std::uint64_t>(static_eval) << 34) |
           (static_cast<std::uint64_t>(node.depth & 0x3F) << 48) |
           (static_cast<std::uint64_t>(node.bound) << 54) |
           (static_cast<std::uint64_t>(node.age) << 56);
  }

  static constexpr Node Unpack(const Hash hash, const std::uint64_t payload) {
    return Node{
        hash,
        Move{static_cast<std::uint16_t>(payload)},
        static_cast<Eval>((static_cast<std::int32_t>(
                              (payload >> 16) & 0x3FFFF) ^
                          0x20000) -
                         0x20000),
        static_cast<Depth>((payload >> 48) & 0x3F),
        static_cast<Bound>((payload >> 54) & 0x3),
        static_cast<std::uint8_t>(payload >> 56),
        static_cast<Eval>((static_cast<int>((payload >> 34) & 0x3FFF) ^
                           0x2000) -
                          0x2000)};
  }

  static constexpr int ReplacementValue(const Node& node,
                                        const std::uint8_t current_age) {
    const auto age = static_cast<std::uint8_t>(current_age - node.age);
    return static_cast<int>(node.depth) - 8 * static_cast<int>(age);
  }

  static constexpr size_t Index(const Hash hash) {
    return static_cast<size_t>((static_cast<__uint128_t>(hash) *
                                kClusterCount) >>
                               64);
  }

  struct MappingDeleter {
    void operator()(Cluster* table) const {
      if (table != nullptr) munmap(table, kMemoryBytes);
    }
  };

  static std::unique_ptr<Cluster[], MappingDeleter> Allocate() {
    void* mapping = mmap(nullptr, kMemoryBytes, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mapping == MAP_FAILED) std::abort();
    if (madvise(mapping, kMemoryBytes, MADV_HUGEPAGE) != 0) {
      munmap(mapping, kMemoryBytes);
      std::abort();
    }
    return std::unique_ptr<Cluster[], MappingDeleter>{
        ::new (mapping) Cluster[kClusterCount]};
  }

  std::unique_ptr<Cluster[], MappingDeleter> table_;
  std::atomic<std::uint64_t> eval_context_{};
};
}  // namespace SimpleChessEngine
