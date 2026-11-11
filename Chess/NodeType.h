#pragma once

#include <utility>

namespace SimpleChessEngine {
enum class NodeType { kPV = 0, kCut = 1, kAll = 2 };

[[nodiscard]] consteval NodeType FirstChildType(const NodeType parent) {
  switch (parent) {
    case NodeType::kPV:
      return NodeType::kPV;
    case NodeType::kCut:
      return NodeType::kAll;
    case NodeType::kAll:
      return NodeType::kCut;
  }
  std::unreachable();
}

[[nodiscard]] consteval NodeType ZeroWindowChildType(const NodeType parent) {
  return parent == NodeType::kCut ? NodeType::kAll : NodeType::kCut;
}

[[nodiscard]] consteval bool IsValidChildType(const NodeType parent,
                                              const NodeType child) {
  switch (child) {
    case NodeType::kPV:
      return parent == NodeType::kPV;
    case NodeType::kCut:
      return parent == NodeType::kPV || parent == NodeType::kAll;
    case NodeType::kAll:
      return parent == NodeType::kCut;
  }
  std::unreachable();
}
}  // namespace SimpleChessEngine
