#pragma once

namespace SimpleChessEngine {
enum class NodeType {
  kPV = 0,
  kCut = 1,
  kAll = 2,
  kNonPv = kCut | kAll,
  kInvalid
};

template <NodeType node_type>
consteval bool IsValidNodeType() {
  return node_type == NodeType::kPV || node_type == NodeType::kCut ||
         node_type == NodeType::kAll;
}
template <NodeType node_type>
consteval NodeType SwapNodeType() {
  if constexpr (node_type == NodeType::kCut) {
    return NodeType::kAll;
  }
  if constexpr (node_type == NodeType::kAll) {
    return NodeType::kCut;
  }
  return NodeType::kInvalid;
}
}  // namespace SimpleChessEngine