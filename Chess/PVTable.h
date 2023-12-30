#pragma once

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
    return table_[(id_ply * kMaxSearchPly) | remaining_depth];
  }
  void SetPV(const Move& move, const size_t id_ply,
             const size_t remaining_depth)
  {
    table_[(id_ply * kMaxSearchPly) | remaining_depth] = move;
  }

 private:
  std::array<Move, kMaxSearchPly * kMaxSearchPly> table_;
};
}  // namespace SimpleChessEngine
