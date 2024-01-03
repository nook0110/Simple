#pragma once

#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine {

template <size_t MaxKillerCount>
class KillerTable {
 public:
  void Clear() 
  {
    used_slots_.fill(0);
  }
  bool TryAdd(const size_t ply, const Move& move) 
  { 
    if (used_slots_[ply] < MaxKillerCount)
      return table_[ply][used_slots_[ply]++] = move, true;
    return false;
  }
  size_t AvailableKillerCount(const size_t ply) const 
  {
    return used_slots_[ply];
  }
  const Move& Get(const size_t ply, const size_t index) const
  {
    assert(index < used_slots_[ply]);
    return table_[ply][index];
  }
 private:
  bool Contains(const size_t ply, const Move& move) {
    for (size_t i = 0; i < used_slots_[ply]; ++i) {
      if (move == table_[ply][i]) return true;
    }
    return false;
  }
  std::array<size_t, kMaxSearchPly> used_slots_ = {};
  std::array<std::array<Move, MaxKillerCount>, kMaxSearchPly> table_ = {};
};

}  // namespace SimpleChessEngine
