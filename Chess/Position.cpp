#include "Position.h"

using namespace SimpleChessEngine;

void Position::DoMove(const Move& move)
{
    auto from = move.from_;
    auto to = move.to_;
    auto piece_to_move = board_[from];
    Player us = side_to_move_;
    Player them = Flip(us);
    RemovePiece(from, us);
    RemovePiece(to, them);
    PlacePiece(to, piece_to_move, us);
    side_to_move_ = Flip(side_to_move_);
}

void Position::UndoMove(const Move& move)
{
    side_to_move_ = Flip(side_to_move_);
    auto from = move.from_;
    auto to = move.to_;
    auto piece_to_move = board_[to];
    auto captured_piece = move.captured_piece_;
    Player us = side_to_move_;
    Player them = Flip(us);
    RemovePiece(to, us);
    PlacePiece(to, captured_piece, them);
    PlacePiece(from, piece_to_move, us);
}
