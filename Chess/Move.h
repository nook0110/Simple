#pragma once
namespace SimpleChessEngine
{
class Move
{
 public:
  using Index = size_t;
  Move() = default;
  Move(Index from, Index to){};

  bool operator==(const Move& other) const { return false; }
  bool operator<(const Move&) const { return false; }
};
}  // namespace SimpleChessEngine