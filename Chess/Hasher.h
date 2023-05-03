#pragma once
namespace SimpleChessEngine
{
class Position;

using Hash = size_t;

class Hasher
{
 public:
  Hash operator()(const Position& position) const { return 0; }
};
}  // namespace SimpleChessEngine
