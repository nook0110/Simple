#include "Position.h"

#include <numeric>
#include <variant>

using namespace SimpleChessEngine;

void Position::DoMove(const Move& move)
{
  en_croissant_square_.reset();
  std::visit([this](const auto& unwrapped_move) { DoMove(unwrapped_move); },
             move);
  side_to_move_ = Flip(side_to_move_);
}

void Position::DoMove(const DefaultMove& move)
{
  const auto [from, to, captured_piece] = move;

  const auto us = side_to_move_;
  const auto them = Flip(us);

  const auto piece_to_move = board_[from];

  RemovePiece(from, us);
  RemovePiece(to, them);
  PlacePiece(to, piece_to_move, us);
}

void Position::DoMove(const DoublePush& move)
{
  const auto from = move.from;

  const auto us = side_to_move_;

  const auto direction = kPawnMoveDirection[static_cast<size_t>(us)];
  const auto to = Shift(Shift(from, direction), direction);

  en_croissant_square_ = std::midpoint(from, to);

  RemovePiece(from, us);
  PlacePiece(to, Piece::kPawn, us);
}

void Position::DoMove(const EnCroissant& move)
{
  const auto [from, to] = move;

  const auto us = side_to_move_;
  const auto them = Flip(us);

  const auto capture_square =
      Shift(to, kPawnMoveDirection[static_cast<size_t>(them)]);

  RemovePiece(from, us);
  RemovePiece(capture_square, them);
  PlacePiece(to, Piece::kPawn, us);
}

void Position::DoMove(const Promotion& move)
{
  const auto [from, to, captured_piece] = static_cast<DefaultMove>(move);
  const auto promoted_to = move.promoted_to;

  const auto us = side_to_move_;
  const auto them = Flip(us);

  RemovePiece(from, us);
  RemovePiece(to, them);
  PlacePiece(to, promoted_to, us);
}

void Position::DoMove(const Castling& move)
{
  const auto [side, king_from, rook_from] = move;

  const auto us = side_to_move_;

  const auto color_idx = static_cast<size_t>(us);
  const auto side_idx = static_cast<size_t>(side);

  RemovePiece(king_from, us);
  RemovePiece(rook_from, us);
  PlacePiece(kKingCastlingDestination[color_idx][side_idx], Piece::kKing, us);
  PlacePiece(kRookCastlingDestination[color_idx][side_idx], Piece::kRook, us);
}

void Position::UndoMove(const Move& move)
{
  side_to_move_ = Flip(side_to_move_);
  std::visit([this](const auto& unwrapped_move) { UndoMove(unwrapped_move); },
             move);
}

void Position::UndoMove(const DefaultMove& move)
{
  const auto [from, to, captured_piece] = move;

  const Player us = side_to_move_;
  const Player them = Flip(us);

  const auto piece_to_move = board_[to];

  RemovePiece(to, us);
  if (!!captured_piece) PlacePiece(to, captured_piece, them);
  PlacePiece(from, piece_to_move, us);
}

void Position::UndoMove(const DoublePush& move)
{
  const auto from = move.from;

  const auto us = side_to_move_;

  const auto to = move.to;

  RemovePiece(to, us);
  PlacePiece(from, Piece::kPawn, us);
}

void Position::UndoMove(const EnCroissant& move)
{
  const auto [from, to] = move;

  const auto us = side_to_move_;
  const auto them = Flip(us);

  const auto capture_square =
      Shift(to, kPawnMoveDirection[static_cast<size_t>(them)]);

  RemovePiece(to, us);
  PlacePiece(capture_square, Piece::kPawn, them);
  PlacePiece(from, Piece::kPawn, us);
}

void Position::UndoMove(const Promotion& move)
{
  const auto [from, to, captured_piece] = static_cast<DefaultMove>(move);

  const auto us = side_to_move_;
  const auto them = Flip(us);

  RemovePiece(to, us);
  if (!!captured_piece) PlacePiece(to, captured_piece, them);
  PlacePiece(from, Piece::kPawn, us);
}

void Position::UndoMove(const Castling& move)
{
  const auto [side, king_from, rook_from] = move;

  const auto us = side_to_move_;

  const auto color_idx = static_cast<size_t>(us);
  const auto side_idx = static_cast<size_t>(side);

  PlacePiece(king_from, Piece::kKing, us);
  PlacePiece(rook_from, Piece::kRook, us);
  RemovePiece(kKingCastlingDestination[color_idx][side_idx], us);
  RemovePiece(kRookCastlingDestination[color_idx][side_idx], us);
}