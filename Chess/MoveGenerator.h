#pragma once

#include <cstdint>
#include <vector>

#include "Move.h"
#include "Position.h"
#include "Utility.h"

namespace SimpleChessEngine
{
/**
 * \brief Class that all possible moves for a given position.
 *
 * \author nook0110
 */
class MoveGenerator
{
 public:
  enum class Type : uint8_t
  {
    kDefault,
    kQuiescence
  };

  MoveGenerator() { moves_.reserve(256); }

  using Moves = std::vector<Move>;

  /**
   * \brief Generates all possible moves for a given position.
   *
   * \param position The position.
   *
   * \return All possible moves for the given position.
   */
  template <Type type>
  [[nodiscard]] Moves GenerateMoves(Position& position) const;

 private:
  [[nodiscard]] static bool IsMoveLegal(Position& position, const Move& move);

  /**
   * \brief Generates all possible moves for a given square.
   *
   * \param position The position.
   * \param moves Container where to add possible moves.
   * \param target Target squares.
   *
   * \return All possible moves for the given square.
   */
  template <Piece piece>
  void GenerateMovesForPiece(Moves& moves, Position& position,
                             Bitboard target) const;

  /**
   * \brief Generates all possible moves for a given square with given piece.
   *
   * \tparam piece The piece.
   * \param moves Container where to add moves.
   * \param position The position.
   * \param from The square.
   * \param target Target squares.
   *
   * \return All possible moves for the given square and piece.
   */
  template <Piece piece>
  void GenerateMovesFromSquare(Moves& moves, Position& position, BitIndex from,
                               Bitboard target) const;

  static void GenerateCastling(Moves& moves, const Position& position);

  mutable Moves moves_;
};

template <MoveGenerator::Type type>
MoveGenerator::Moves MoveGenerator::GenerateMoves(Position& position) const
{
  moves_.clear();

  const auto us = position.GetSideToMove();
  const auto them = Flip(us);

  auto target = ~position.GetPieces(us);

  if constexpr (type == Type::kQuiescence)
  {
    target &= position.GetPieces(Flip(us));
  }

  const auto king_square = position.GetKingSquare(us);
  const auto king_attacker =
      position.Attackers(king_square) & position.GetPieces(them);

  // Double-check check
  if (king_attacker.MoreThanOne())
  {
    GenerateMovesForPiece<Piece::kKing>(moves_, position, target);
    return moves_;
  }

  // compute pins
  position.ComputePins(us);

  const auto king_target = target;
  auto pawn_target = target;

  if constexpr (type == Type::kQuiescence)
  {
    pawn_target |= (kRankBB[0] | kRankBB[7]);
  }
  // is in check
  if (king_attacker.Any())
  {
    const auto attacker = king_attacker.GetFirstBit();
    const auto ray =
        Between(king_square, attacker) | GetBitboardOfSquare(attacker);
    target &= ray;
    pawn_target &= ray;
  }

  GenerateMovesForPiece<Piece::kPawn>(moves_, position, pawn_target);

  std::erase_if(moves_, [&position](const Move& move)
                { return !IsMoveLegal(position, move); });

  // generate moves for piece
  GenerateMovesForPiece<Piece::kKing>(moves_, position, king_target);
  GenerateMovesForPiece<Piece::kKnight>(moves_, position, target);
  GenerateMovesForPiece<Piece::kBishop>(moves_, position, target);
  GenerateMovesForPiece<Piece::kRook>(moves_, position, target);
  GenerateMovesForPiece<Piece::kQueen>(moves_, position, target);

  GenerateCastling(moves_, position);

  // return moves
  return moves_;
}

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
inline void MoveGenerator::GenerateMovesForPiece<Piece::kPawn>(
    Moves& moves, Position& position, const Bitboard target) const
{
  const auto us = position.GetSideToMove();
  const auto us_idx = static_cast<size_t>(us);
  const auto them = Flip(us);
  const auto them_idx = static_cast<size_t>(them);

  const auto pawns = position.GetPiecesByType<Piece::kPawn>(us);
  const auto promotion_rank = us == Player::kWhite ? kRankBB[6] : kRankBB[1];
  const auto direction = kPawnMoveDirection[us_idx];
  const auto opposite_direction = kPawnMoveDirection[them_idx];

  const auto non_promoting_pawns = pawns & ~promotion_rank;

  const auto valid_squares = ~position.GetAllPieces();

  const auto third_rank = us == Player::kWhite ? kRankBB[2] : kRankBB[5];

  auto push = Shift(non_promoting_pawns, direction) & valid_squares;

  const auto double_push_pawns = push & third_rank;

  push &= target;
  while (push.Any())
  {
    const auto to = push.PopFirstBit();

    const auto from = Shift(to, opposite_direction);

    moves.emplace_back(PawnPush{from, to});
  }

  auto double_push = Shift(double_push_pawns, direction) & valid_squares;

  double_push &= target;
  while (double_push.Any())
  {
    const auto to = double_push.PopFirstBit();

    const auto from = Shift(Shift(to, opposite_direction), opposite_direction);

    moves.emplace_back(DoublePush{from, to});
  }

  static constexpr std::array cant_attack_files = {kFileBB[0], kFileBB[7]};

  const auto attacks =
      (us == Player::kWhite)
          ? std::array{Compass::kNorthWest, Compass::kNorthEast}
          : std::array{Compass::kSouthWest, Compass::kSouthEast};

  const auto opposite_attacks =
      (us == Player::kWhite)
          ? std::array{Compass::kSouthEast, Compass::kSouthWest}
          : std::array{Compass::kNorthEast, Compass::kNorthWest};

  const auto enemy_pieces = position.GetPieces(them);

  const auto en_croissant_square = position.GetEnCroissantSquare();

  const std::array attacks_to = {
      Shift(non_promoting_pawns & ~cant_attack_files.front(), attacks.front()),
      Shift(non_promoting_pawns & ~cant_attack_files.back(), attacks.back())};

  for (size_t attack_direction = 0; attack_direction < attacks.size();
       ++attack_direction)
  {
    auto attack_squares = attacks_to[attack_direction] & target & enemy_pieces;

    while (attack_squares.Any())
    {
      const auto to = attack_squares.PopFirstBit();

      const auto from = Shift(to, opposite_attacks[attack_direction]);

      moves.emplace_back(DefaultMove{from, to, position.GetPiece(to)});
    }
  }

  if (en_croissant_square)
  {
    const auto en_croissant_bitboard =
        GetBitboardOfSquare(en_croissant_square.value());
    for (size_t attack_direction = 0; attack_direction < attacks.size();
         ++attack_direction)
    {
      auto attack_to = attacks_to[attack_direction] & en_croissant_bitboard;
      if (attack_to.Any())
      {
        const auto to = en_croissant_square.value();
        moves.emplace_back(
            EnCroissant{Shift(to, opposite_attacks[attack_direction]), to});
      }
    }
  }

  const auto promoting_pawns = pawns & promotion_rank;

  auto promotion_push =
      Shift(promoting_pawns, direction) & valid_squares & target;

  while (promotion_push.Any())
  {
    const auto to = promotion_push.PopFirstBit();

    const auto from = Shift(to, opposite_direction);

    moves.emplace_back(
        Promotion{{from, to, position.GetPiece(to)}, Piece::kQueen});
    moves.emplace_back(
        Promotion{{from, to, position.GetPiece(to)}, Piece::kKnight});
    moves.emplace_back(
        Promotion{{from, to, position.GetPiece(to)}, Piece::kRook});
    moves.emplace_back(
        Promotion{{from, to, position.GetPiece(to)}, Piece::kBishop});
  }

  for (size_t attack_direction = 0; attack_direction < attacks.size();
       ++attack_direction)
  {
    auto attack_squares =
        Shift(promoting_pawns & ~cant_attack_files[attack_direction],
              attacks[attack_direction]) &
        target & enemy_pieces;

    while (attack_squares.Any())
    {
      const auto to = attack_squares.PopFirstBit();

      const auto from = Shift(to, opposite_attacks[attack_direction]);

      moves.emplace_back(
          Promotion{{from, to, position.GetPiece(to)}, Piece::kKnight});
      moves.emplace_back(
          Promotion{{from, to, position.GetPiece(to)}, Piece::kBishop});
      moves.emplace_back(
          Promotion{{from, to, position.GetPiece(to)}, Piece::kRook});
      moves.emplace_back(
          Promotion{{from, to, position.GetPiece(to)}, Piece::kQueen});
    }
  }
}

template <>
inline void MoveGenerator::GenerateMovesForPiece<Piece::kKing>(
    Moves& moves, Position& position, Bitboard target) const
{
  const auto us = position.GetSideToMove();
  const auto them = Flip(us);

  const auto king_pos = position.GetKingSquare(us);
  const auto king_mask = GetBitboardOfSquare(king_pos);

  const auto occupancy = position.GetAllPieces() ^ king_mask;

  // we prevent the king from going to squares attacked by enemy pieces

  target &= ~position.GetAllPawnAttacks(Flip(us));

  Bitboard attackers = position.GetPiecesByType<Piece::kKnight>(them);
  while (attackers.Any())
  {
    target &= ~AttackTable<Piece::kKnight>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kBishop>(them);
  while (attackers.Any())
  {
    target &= ~AttackTable<Piece::kBishop>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kRook>(them);
  while (attackers.Any())
  {
    target &= ~AttackTable<Piece::kRook>::GetAttackMap(attackers.PopFirstBit(),
                                                       occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kQueen>(them);
  while (attackers.Any())
  {
    target &= ~AttackTable<Piece::kQueen>::GetAttackMap(attackers.PopFirstBit(),
                                                        occupancy);
  }

  target &= ~AttackTable<Piece::kKing>::GetAttackMap(
      position.GetKingSquare(them), occupancy);

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

  // if the piece is pinned we can only move in pin direction
  if (position.GetIrreversibleData()
          .blockers[static_cast<size_t>(side_to_move)]
          .Test(from))
  {
    target &= Ray(position.GetKingSquare(side_to_move), from);
  }

  // we move only in target squares
  auto valid_moves = attacks & target;

  while (valid_moves.Any())
  {
    const auto to = valid_moves.PopFirstBit();

    const auto move = DefaultMove{from, to, position.GetPiece(to)};
    moves.emplace_back(move);
  }
}
}  // namespace SimpleChessEngine
