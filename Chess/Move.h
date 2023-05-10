#pragma once
namespace SimpleChessEngine
{
class Move
{
 public:
  using Index = size_t;
  Move(Index from, Index to);
};
}  // namespace SimpleChessEngine