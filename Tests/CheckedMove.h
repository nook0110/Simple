#pragma once

#include <cstdlib>
#include <string>

#include "MoveFactory.h"
#include "Position.h"

namespace SimpleChessEngine {

inline LegalMove CheckedMove(Position& position, const Move& move) {
  const auto pseudo = position.TryPseudoLegal(move);
  if (!pseudo) std::abort();
  const auto legal = position.TryLegal(*pseudo);
  if (!legal) std::abort();
  return *legal;
}

inline LegalMove CheckedMove(Position& position, const std::string& text) {
  return CheckedMove(position, MoveFactory{}(position, text));
}

}
