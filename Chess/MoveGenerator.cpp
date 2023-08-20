#include "MoveGenerator.h"

#include <cassert>

#include "Attacks.h"

using namespace SimpleChessEngine;

MoveGenerator::Moves MoveGenerator::operator()(Position& position) const
{
  // create moves, max amount moves 218
  Moves moves;
  constexpr size_t kMaxMovesAmount = 218;
  moves.reserve(kMaxMovesAmount);

  // get all pieces
  auto pieces = position.GetAllPieces();

  // generate moves for each piece
  while (const auto from = pieces.GetFirstBit())
  {
    // get first piece
    pieces.reset(*from);

    // generate moves for piece
    auto moves_for_piece = GenerateMovesForPiece(position, *from);

    // add moves
    moves.insert(moves.end(), moves_for_piece.begin(), moves_for_piece.end());
  }

  // return moves
  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateMovesForPiece(
    Position& position, const BitIndex from) const
{
  // create moves, max amount moves 218
  Moves moves;
  constexpr size_t kMaxMovesAmount = 218;
  moves.reserve(kMaxMovesAmount);

  // get piece and side to move
  const auto piece = position.GetPiece(from);
  const auto side_to_move = position.GetSideToMove();

  // check if piece exists
  assert((position.GetAllPieces()[from]));

  switch (piece)
  {
    case Piece::kPawn:
      // generate pawn moves
      moves = GenerateMoves<Piece::kPawn>(position, from);
      break;
    case Piece::kKnight:
      // generate knight moves
      moves = GenerateMoves<Piece::kKnight>(position, from);
      break;
    case Piece::kBishop:
      // generate bishop moves
      moves = GenerateMoves<Piece::kBishop>(position, from);
      break;
    case Piece::kRook:
      // generate rook moves
      moves = GenerateMoves<Piece::kRook>(position, from);
      break;
    case Piece::kQueen:
      // generate queen moves
      moves = GenerateMoves<Piece::kQueen>(position, from);
      break;
    case Piece::kKing:
      // generate king moves
      moves = GenerateMoves<Piece::kKing>(position, from);
      break;
    case Piece::kNone:
      assert(false);
      break;
    default:
      break;
  }

  // return moves
  return moves;
}

template <Piece piece>
MoveGenerator::Moves MoveGenerator::GenerateMoves(Position& position,
                                                  const BitIndex from) const
{
  Moves moves;
  constexpr size_t kMaxMoves = 64;
  moves.reserve(kMaxMoves);

  // get all squares that piece attacks
  const auto attacks =
      AttackTable<piece>::GetAttackMap(from, position.GetAllPieces());

  // get whose move is now
  const auto side_to_move = position.GetSideToMove();

  // get our pieces
  const auto& our_pieces = position.GetPieces(side_to_move);

  // remove moves into our pieces
  auto valid_moves = attacks & (~our_pieces);

  while (const auto to = valid_moves.GetFirstBit())
  {
    valid_moves.reset(to.value());

    // create move
    Move move{from, to.value(), position.GetPiece(to.value())};

    // do move
    position.DoMove(move);

    // check if move is valid
    if (!position.IsUnderCheck(side_to_move))
    {
      static_assert(std::is_trivially_copyable_v<decltype(move)>);
      moves.push_back(move);
    }

    // undo move
    position.UndoMove(move);
  }

  // return moves
  return moves;
}

template MoveGenerator::Moves MoveGenerator::GenerateMoves<Piece::kKnight>(
    Position& position, BitIndex from) const;
template MoveGenerator::Moves MoveGenerator::GenerateMoves<Piece::kBishop>(
    Position& position, BitIndex from) const;
template MoveGenerator::Moves MoveGenerator::GenerateMoves<Piece::kRook>(
    Position& position, BitIndex from) const;
template MoveGenerator::Moves MoveGenerator::GenerateMoves<Piece::kQueen>(
    Position& position, BitIndex from) const;
template MoveGenerator::Moves MoveGenerator::GenerateMoves<Piece::kKing>(
    Position& position, BitIndex from) const;