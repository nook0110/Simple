#pragma once

#include <atomic>
#include <cstdint>

namespace SimpleChessEngine {
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

class NodeCounter {
 public:
  [[gnu::always_inline]] void Add(const std::uint64_t nodes) noexcept {
    count_.store(count_.load(std::memory_order_relaxed) + nodes,
                 std::memory_order_relaxed);
  }

  [[gnu::always_inline]] void Reset() noexcept {
    count_.store(0, std::memory_order_relaxed);
  }

  [[gnu::always_inline]] [[nodiscard]] std::uint64_t Get() const noexcept {
    return count_.load(std::memory_order_relaxed);
  }

 private:
  std::atomic<std::uint64_t> count_{0};
};
}  // namespace SimpleChessEngine
