#pragma once

#include <cstdlib>
#include <string>

#include "MoveFactory.h"
#include "Position.h"

namespace SimpleChessEngine {

inline LegalMove CheckedMove(Position& position, const std::string& text) {
  const auto pseudo = position.TryPseudoLegal(MoveFactory{}(position, text));
  if (!pseudo) std::abort();
  const auto legal = position.TryLegal(*pseudo);
  if (!legal) std::abort();
  return *legal;
}

}
