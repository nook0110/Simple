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

  std::optional<Move>& operator[](const Position& position)
  {
    return table_[hasher_(position)];
  }

  const std::optional<Move>& operator[](const Position& position) const
  {
    return table_[hasher_(position)];
  }

 private:
  constexpr static size_t kSize = 10;

  std::array<std::optional<Move>, kSize> table_;
  Hasher hasher_;
};
}  // namespace SimpleChessEngine
