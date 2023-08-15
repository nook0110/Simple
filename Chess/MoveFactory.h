#pragma once

#include "Move.h"

namespace SimpleChessEngine
{
struct MoveFactory
{
  Move operator()(const std::string& move) const;
};

inline Move MoveFactory::operator()(const std::string& move) const
{
  return {};
}
}  // namespace SimpleChessEngine
