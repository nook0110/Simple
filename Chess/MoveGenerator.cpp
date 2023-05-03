#include "MoveGenerator.h"

#include <cassert>

using namespace SimpleChessEngine;

MoveGenerator::Moves MoveGenerator::operator()(const Position& position) const
{
  // create moves, max amount moves 218
  Moves moves;
  constexpr size_t kMaxMovesAmount = 218;
  moves.reserve(kMaxMovesAmount);

  // get all pieces
  auto pieces = position.GetPieces();

  // generate moves for each piece
  while (pieces.any())
  {
    const auto from = pieces.GetFirstBit();
    pieces.reset(from);

    auto moves_for_piece = GenerateMovesForPiece(position, from);
    moves.insert(moves.end(), moves_for_piece.begin(), moves_for_piece.end());
  }

  // return moves
  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateMovesForPiece(
    const Position& position, BitIndex from) const
{
  // create moves, max amount moves 218
  Moves moves;
  constexpr size_t kMaxMovesAmount = 218;
  moves.reserve(kMaxMovesAmount);

  // get piece and side to move
  const auto piece = position.GetPiece(from);
  const auto side_to_move = position.GetSideToMove();

  // check if piece exists
  assert((position.GetPieces()[from]));

  switch (piece)
  {
    case Piece::kPawn:
      // generate pawn moves
      moves = GeneratePawnMoves(position, from);
      break;
    case Piece::kKnight:
      moves = GenerateKnightMoves(position, from);
      break;
    case Piece::kBishop:
      moves = GenerateBishopMoves(position, from);
      break;
    case Piece::kRook:
      moves = GenerateRookMoves(position, from);
      break;
    case Piece::kQueen:
      moves = GenerateQueenMoves(position, from);
      break;
    case Piece::kKing:
      moves = GenerateKingMoves(position, from);
      break;
    default:
      break;
  }

  return moves;
}

MoveGenerator::Moves MoveGenerator::GeneratePawnMoves(const Position& position,
                                                      BitIndex from) const
{
  Moves moves;
  constexpr size_t kMaxPawnMovesAmount = 4;
  moves.reserve(kMaxPawnMovesAmount);

  // TODO: generate pawn moves
  assert((false));

  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateKnightMoves(
    const Position& position, BitIndex from) const
{
  Moves moves;
  constexpr size_t kMaxKnightMoves = 8;
  moves.reserve(kMaxKnightMoves);

  // TODO: generate knight moves
  assert((false));

  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateBishopMoves(
    const Position& position, BitIndex from) const
{
  Moves moves;
  constexpr size_t kMaxBishopMoves = 13;
  moves.reserve(kMaxBishopMoves);

  // TODO: generate bishop moves
  assert((false));

  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateRookMoves(const Position& position,
                                                      BitIndex from) const
{
  return Moves{};
}

MoveGenerator::Moves MoveGenerator::GenerateQueenMoves(const Position& position,
                                                       BitIndex from) const
{
  return Moves{};
}

MoveGenerator::Moves MoveGenerator::GenerateKingMoves(const Position& position,
                                                      BitIndex from) const
{
  return Moves{};
}