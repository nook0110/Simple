#include "Position.h"

using namespace SimpleChessEngine;

void Position::DoMove(const Move& move)
{
  const auto from = move.from;
  const auto to = move.to;
  const auto piece_to_move = board_[from];

  const Player us = side_to_move_;
  const Player them = Flip(us);
  RemovePiece(from, us);
  RemovePiece(to, them);
  PlacePiece(to, piece_to_move, us);
  side_to_move_ = Flip(side_to_move_);
}

void Position::UndoMove(const Move& move)
{
  side_to_move_ = Flip(side_to_move_);
  const auto from = move.from;
  const auto to = move.to;
  const auto piece_to_move = board_[to];
  auto captured_piece = move.captured_piece;
  const Player us = side_to_move_;
  const Player them = Flip(us);
  RemovePiece(to, us);
  PlacePiece(to, captured_piece, them);
  PlacePiece(from, piece_to_move, us);
}