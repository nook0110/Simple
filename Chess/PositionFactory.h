#pragma once
#include "Position.h"

namespace SimpleChessEngine
{
class PositionFactory
{
  Position operator()(
      const std::string& fen =
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
};

inline Position PositionFactory::operator()(const std::string& fen) {}
}  // namespace SimpleChessEngine
