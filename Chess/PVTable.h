#pragma once

#include <algorithm>

#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine {
class PVTable {
 public:
  [[nodiscard]] bool CheckPV(const size_t ply) const {
    return GetPV(ply) != Move{};
  }

  [[nodiscard]] const Move& GetPV(const size_t ply) const {
    return table_[ply];
  }

  void SetPV(const Move& move, const size_t ply) {
    table_[ply * kMaxSearchPly] = move;
  }

  void EndPV(const size_t ply) { SetPV({}, ply); }

  void FetchNextLayer(const size_t ply, const size_t remaining_depth) {
    std::copy(
        table_.begin() + (ply + 1) * kMaxSearchPly,
        table_.begin() + ((ply + 1) * kMaxSearchPly | (remaining_depth - 1)),
        table_.begin() + ply * kMaxSearchPly + 1);
  }

 private:
  std::array<Move, kMaxSearchPly * kMaxSearchPly> table_;
};
}  // namespace SimpleChessEngine
