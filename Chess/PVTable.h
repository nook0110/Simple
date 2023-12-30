#pragma once

#include "Move.h"
#include "Utility.h"

#include <algorithm>

namespace SimpleChessEngine
{
class PVTable
{
 public:
  [[nodiscard]] const Move& GetPV(const size_t id_ply,
                                  const size_t remaining_depth) const
  {
    return table_[(id_ply * kMaxSearchPly) | remaining_depth];
  }

  void SetPV(const Move& move, const size_t id_ply,
             const size_t remaining_depth)
  {
    table_[(id_ply * kMaxSearchPly) | remaining_depth] = move;
  }

  void FetchNextLayer(const size_t ply)
  {
    std::copy(table_.begin() + (ply - 1) * kMaxSearchPly, table_.begin() + (((ply - 1) * kMaxSearchPly) | (ply - 1)), table_.begin() + ((ply * kMaxSearchPly) | 1));
  }

 private:
  std::array<Move, kMaxSearchPly * kMaxSearchPly> table_;
};
}  // namespace SimpleChessEngine
