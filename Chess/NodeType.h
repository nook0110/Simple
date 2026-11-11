#pragma once

namespace SimpleChessEngine {
enum class NodeType {
  kPV = 0,
  kCut = 1,
  kAll = 2,
  kInvalid
};

template <NodeType node_type>
consteval bool IsValidNodeType() {
  return node_type == NodeType::kPV || node_type == NodeType::kCut ||
         node_type == NodeType::kAll;
}
}  // namespace SimpleChessEngine