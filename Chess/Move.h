#pragma once
namespace SimpleChessEngine
{
struct Move
{
  using Index = BitIndex;
  Move() = default;
  Move(Index from, Index to)
      : from_(from), to_(to)
  {}

  bool operator==(const Move& other) const { return false; }
  bool operator<(const Move&) const { return false; }

  const Index from_;
  const Index to_;
  const Piece captured_piece_;
};
}  // namespace SimpleChessEngine