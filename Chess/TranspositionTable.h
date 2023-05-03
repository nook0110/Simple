#pragma once
#include <array>
#include <cassert>
#include <optional>

#include "Hasher.h"
#include "Move.h"
#include "Position.h"
namespace SimpleChessEngine
{
class TranspositionTable
{
 public:
  struct Node
  {
    Move move;
    // sth else...
  };

  [[nodiscard]] bool Contains(const Position& position) const { return false; }

  Move& operator[](const Position& position)
  {
    assert(table_[hasher_(position)].has_value());
    return table_[hasher_(position)].value();
  }

  const Move& operator[](const Position& position) const
  {
    assert(table_[hasher_(position)].has_value());
    return table_[hasher_(position)].value();
  }

 private:
  constexpr static size_t kSize = 10;

  std::array<std::optional<Move>, kSize> table_;
  Hasher hasher_;
};
}  // namespace SimpleChessEngine
