#include "Position.h"

#include <variant>

using namespace SimpleChessEngine;

void Position::DoMove(const Move& move)
{
  std::visit([this](const auto& unwrapped_move) { DoMove(unwrapped_move); },
             move);
}

void Position::DoMove(const DefaultMove& move) {}

void Position::DoMove(const EnCroissant& move) {}

void Position::DoMove(const Promotion& move) {}

void Position::DoMove(const Castling& move) {}

void Position::UndoMove(const Move& move)
{
  std::visit([this](const auto& unwrapped_move) { DoMove(unwrapped_move); },
             move);
}

void Position::UndoMove(const DefaultMove& move) {}

void Position::UndoMove(const EnCroissant& move) {}

void Position::UndoMove(const Promotion& move) {}

void Position::UndoMove(const Castling& move) {}