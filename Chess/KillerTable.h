#pragma once

#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine {
template <size_t MaxKillerCount>
class KillerTable {
 public:
  void Clear() { killer_total_count_.fill(0); }
  void TryAdd(const size_t ply, const Move& move) {
    if (!Contains(ply, move)) {
      table_[ply][killer_total_count_[ply]++ % MaxKillerCount] = move;
    }
  }
  size_t AvailableKillerCount(const size_t ply) const {
    return std::min(killer_total_count_[ply], MaxKillerCount);
  }
  const Move& Get(const size_t ply, const size_t index) const {
    assert(index < killer_total_count_[ply] && index < MaxKillerCount);
    return table_[ply][index];
  }

 private:
  bool Contains(const size_t ply, const Move& move) {
    for (size_t i = 0; i < std::min(killer_total_count_[ply], MaxKillerCount);
         ++i) {
      if (move == table_[ply][i]) return true;
    }
    return false;
  }
  std::array<size_t, kMaxSearchPly> killer_total_count_ = {};
  std::array<std::array<Move, MaxKillerCount>, kMaxSearchPly> table_ = {};
};
}  // namespace SimpleChessEngine
