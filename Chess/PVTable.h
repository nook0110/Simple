#pragma once

#include <algorithm>

#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine
{
class PVTable
{
 public:
  [[nodiscard]] const Move& GetPV(const size_t id_ply,
                                  const size_t remaining_depth) const
  {
    return table_[(id_ply * kMaxSearchPly) | (remaining_depth - 1)];
  }

  void SetPV(const Move& move, const size_t id_ply,
             const size_t remaining_depth)
  {
    table_[(id_ply * kMaxSearchPly) | (remaining_depth - 1)] = move;
  }

  void FetchNextLayer(const size_t ply)
  {
    std::copy(table_.begin() + (ply - 1) * kMaxSearchPly,
              table_.begin() + (((ply - 1) * kMaxSearchPly) | (ply - 1)),
              table_.begin() + ply * kMaxSearchPly);
  }

 private:
  std::array<Move, kMaxSearchPly * kMaxSearchPly> table_;
};
}  // namespace SimpleChessEngine
