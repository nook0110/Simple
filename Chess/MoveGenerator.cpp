#include "MoveGenerator.h"

#include <cassert>

#include "Attacks.h"

using namespace SimpleChessEngine;

template <MoveGenerator::Type type>
MoveGenerator::Moves MoveGenerator::GenerateMoves(Position& position)
{
  moves_.clear();

  const auto us = position.GetSideToMove();

  auto target = Bitboard{}.Set();

  if constexpr (type == Type::kQuiescence)
  {
    target &= position.GetPieces(Flip(us));
  }

  // generate moves for piece
  GenerateMovesForPiece<Piece::kPawn>(moves_, position, target);
  GenerateMovesForPiece<Piece::kKnight>(moves_, position, target);
  GenerateMovesForPiece<Piece::kBishop>(moves_, position, target);
  GenerateMovesForPiece<Piece::kQueen>(moves_, position, target);
  GenerateMovesForPiece<Piece::kKing>(moves_, position, target);

  erase_if(moves_, [&position](const Move& move)
           { return !IsMoveValid(position, move); });
  GenerateCastling(moves_, position);

  // return moves
  return moves_;
}

template MoveGenerator::Moves
MoveGenerator::GenerateMoves<MoveGenerator::Type::kDefault>(Position& position);
template MoveGenerator::Moves MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kQuiescence>(Position& position);

template <Piece piece>
void MoveGenerator::GenerateMovesForPiece(Moves& moves, Position& position,
                                          const Bitboard target) const
{
  static_assert(piece != Piece::kPawn && piece != Piece::kKing);

  const auto us = position.GetSideToMove();
  Bitboard pieces = position.GetPiecesByType<piece>(us);

  while (pieces.Any())
  {
    const auto from = pieces.PopFirstBit();
    GenerateMovesFromSquare<piece>(moves, position, from, target);
  }
}

template <>
void MoveGenerator::GenerateMovesForPiece<Piece::kPawn>(
    Moves& moves, Position& position, const Bitboard target) const
{
  const auto us = position.GetSideToMove();

  const auto pawns = position.GetPiecesByType<Piece::kPawn>(us);
  const auto promotion_rank = us == Player::kWhite ? kRankBB[6] : kRankBB[1];
  const auto direction =
      us == Player::kWhite ? Compass::kNorth : Compass::kSouth;

  const auto opposite_direction =
      us == Player::kWhite ? Compass::kSouth : Compass::kNorth;

  const auto not_promoting_pawns = pawns & ~promotion_rank;
  const auto promoting_pawns = pawns & promotion_rank;

  const auto valid_squares = ~position.GetAllPieces() & target;

  const auto third_rank = us == Player::kWhite ? kRankBB[2] : kRankBB[5];

  auto push = Shift(not_promoting_pawns, direction) & valid_squares;

  const auto double_push_pawns = push & third_rank;

  while (push.Any())
  {
    const auto to = push.PopFirstBit();

    const auto from = Shift(to, opposite_direction);

    moves.emplace_back(DefaultMove{from, to});
  }

  auto double_push = Shift(double_push_pawns, direction) & valid_squares;

  while (double_push.Any())
  {
    const auto to = double_push.PopFirstBit();

    const auto from = Shift(Shift(to, opposite_direction), opposite_direction);

    moves.emplace_back(PawnPush{from, to});
  }
}

template <>
void MoveGenerator::GenerateMovesForPiece<Piece::kKing>(
    Moves& moves, Position& position, const Bitboard target) const
{
  const auto us = position.GetSideToMove();

  const auto king_pos = position.GetKingSquare(us);

  GenerateMovesFromSquare<Piece::kKing>(moves, position, king_pos, target);
}

template <Piece piece>
void MoveGenerator::GenerateMovesFromSquare(Moves& moves, Position& position,
                                            const BitIndex from,
                                            Bitboard target) const
{
  assert(position.GetPiece(from) == piece);

  // get all squares that piece attacks
  const auto attacks =
      AttackTable<piece>::GetAttackMap(from, position.GetAllPieces());

  // get whose move is now
  const auto side_to_move = position.GetSideToMove();

  // get our pieces
  const auto& our_pieces = position.GetPieces(side_to_move);

  // remove moves into our pieces
  auto valid_moves = attacks & ~our_pieces & target;

  while (valid_moves.Any())
  {
    const auto to = valid_moves.PopFirstBit();

    const auto move = DefaultMove{from, to, position.GetPiece(to)};
    moves.emplace_back(move);
  }

  if constexpr (piece == Piece::kKing)
  {
    GenerateCastling(moves, position);
  }
}

void MoveGenerator::GenerateCastling(Moves& moves, const Position& position)
{
  if (position.IsUnderCheck())
  {
    return;
  }

  const auto side_to_move = position.GetSideToMove();

  const auto king_square = position.GetKingSquare(side_to_move);

  for (const auto castling_side :
       {Castling::CastlingSide::k00, Castling::CastlingSide::k000})
  {
    if (position.CanCastle(castling_side))
    {
      const auto rook_square =
          position.GetCastlingRookSquare(side_to_move, castling_side);
      moves.emplace_back(Castling{castling_side, king_square, rook_square});
    }
  }
}