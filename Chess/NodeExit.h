#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <type_traits>

#include "Evaluation.h"

namespace SimpleChessEngine {
class [[nodiscard]] NodeExit {
 public:
  static constexpr NodeExit Continue() { return NodeExit{Kind::kContinue}; }
  static constexpr NodeExit Cutoff(const Eval value) {
    return NodeExit{Kind::kCutoff, value};
  }
  static constexpr NodeExit Abort() { return NodeExit{Kind::kAbort}; }

  [[nodiscard]] constexpr explicit operator bool() const {
    return kind_ != Kind::kContinue;
  }

  [[nodiscard]] constexpr SearchResult Result() const {
    assert(kind_ != Kind::kContinue);
    if (kind_ == Kind::kCutoff) return value_;
    return std::nullopt;
  }

  friend constexpr bool operator==(NodeExit, NodeExit) = default;

 private:
  enum class Kind : std::uint8_t { kContinue, kCutoff, kAbort };

  constexpr explicit NodeExit(const Kind kind, const Eval value = {})
      : value_(value), kind_(kind) {}

  Eval value_;
  Kind kind_;
};

static_assert(sizeof(NodeExit) <= sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<NodeExit>);
}  // namespace SimpleChessEngine
