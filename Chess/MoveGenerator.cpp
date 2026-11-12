#include "MoveGenerator.h"

#include <algorithm>
#include <cassert>

#include "Attacks.h"
#include "BitBoard.h"

namespace SimpleChessEngine {
template <>
void MoveGenerator::GenerateMovesForPiece<Piece::kKing>(
    PseudoLegalMoves& moves, Position& position, Bitboard target) const;

template MoveGenerator::LegalMoves MoveGenerator::GenerateLegalMoves<
    MoveGenerator::Type::kAll>(Position& position) const;
template MoveGenerator::LegalMoves MoveGenerator::GenerateLegalMoves<
    MoveGenerator::Type::kQuiescence>(Position& position) const;


MoveGenerator::~MoveGenerator() = default;

void MoveGenerator::GenerateCastling(PseudoLegalMoves& moves,
                                     const Position& position) {
  if (position.IsUnderCheck()) {
    return;
  }

  const auto side_to_move = position.GetSideToMove();
  const auto king_square = position.GetKingSquare(side_to_move);

  for (const auto castling_side : {CastlingSide::k00, CastlingSide::k000}) {
    if (position.CanPseudoCastle(castling_side)) {
      const auto king_to = kKingCastlingDestination[side_to_move][castling_side];

      moves.unchecked_push_back(PseudoLegalMove(
          Move::Make<MoveType::kCastling>(king_square, king_to)));
    }
  }
}

template <Piece piece>
void MoveGenerator::GenerateMovesForPiece(PseudoLegalMoves& moves,
                                          Position& position,
                                          const Bitboard target) const {
  static_assert(piece != Piece::kPawn && piece != Piece::kKing);

  const auto us = position.GetSideToMove();
  Bitboard pieces = position.GetPiecesByType<piece>(us);

  while (pieces.Any()) {
    const auto from = pieces.PopFirstBit();
    GenerateMovesFromSquare<piece>(moves, position, from, target);
  }
}

template <>
void MoveGenerator::GenerateMovesForPiece<Piece::kPawn>(
    PseudoLegalMoves& moves, Position& position, const Bitboard target) const {
  const auto us = position.GetSideToMove();
  const auto them = Flip(us);

  const auto pawns = position.GetPiecesByType<Piece::kPawn>(us);
  const auto promotion_rank = us == Player::kWhite ? kRankBB[6] : kRankBB[1];
  const auto direction = kPawnMoveDirection[us];
  const auto opposite_direction = kPawnMoveDirection[them];

  const auto non_promoting_pawns = pawns & ~promotion_rank;

  const auto valid_squares = ~position.GetAllPieces();

  const auto third_rank = us == Player::kWhite ? kRankBB[2] : kRankBB[5];

  auto push = Shift(non_promoting_pawns, direction) & valid_squares;

  const auto double_push_pawns = push & third_rank;

  push &= target;
  while (push.Any()) {
    const auto to = push.PopFirstBit();
    const auto from = Shift(to, opposite_direction);
    moves.unchecked_push_back(PseudoLegalMove(Move(from, to)));
  }

  auto double_push = Shift(double_push_pawns, direction) & valid_squares;

  double_push &= target;
  while (double_push.Any()) {
    const auto to = double_push.PopFirstBit();
    const auto from = Shift(Shift(to, opposite_direction), opposite_direction);
    moves.unchecked_push_back(PseudoLegalMove(Move(from, to)));
  }

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

  const std::array attacks_to = {Shift(non_promoting_pawns, attacks.front()),
                                 Shift(non_promoting_pawns, attacks.back())};

  for (size_t attack_direction = 0; attack_direction < attacks.size();
       ++attack_direction) {
    auto attack_squares = attacks_to[attack_direction] & target & enemy_pieces;

    while (attack_squares.Any()) {
      const auto to = attack_squares.PopFirstBit();
      const auto from = Shift(to, opposite_attacks[attack_direction]);
      moves.unchecked_push_back(PseudoLegalMove(Move(from, to)));
    }
  }

  if (en_croissant_square) {
    const auto en_croissant_bitboard =
        SingleSquare(en_croissant_square.value());
    for (size_t attack_direction = 0; attack_direction < attacks.size();
         ++attack_direction) {
      auto attack_to = attacks_to[attack_direction] & en_croissant_bitboard;
      if (attack_to.Any()) {
        const auto to = en_croissant_square.value();
        const auto from = Shift(to, opposite_attacks[attack_direction]);
        moves.unchecked_push_back(PseudoLegalMove(Move::Make<MoveType::kEnPassant>(from, to)));
      }
    }
  }

  const auto promoting_pawns = pawns & promotion_rank;

  auto promotion_push =
      Shift(promoting_pawns, direction) & valid_squares & target;

  while (promotion_push.Any()) {
    const auto to = promotion_push.PopFirstBit();
    const auto from = Shift(to, opposite_direction);

    moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kQueen)));
    moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kKnight)));
    moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kRook)));
    moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kBishop)));
  }

  for (size_t attack_direction = 0; attack_direction < attacks.size();
       ++attack_direction) {
    auto attack_squares = Shift(promoting_pawns, attacks[attack_direction]) &
                          target & enemy_pieces;

    while (attack_squares.Any()) {
      const auto to = attack_squares.PopFirstBit();
      const auto from = Shift(to, opposite_attacks[attack_direction]);

      moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kQueen)));
      moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kKnight)));
      moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kRook)));
      moves.unchecked_push_back(PseudoLegalMove(
        Move::Make<MoveType::kPromotion>(from, to, Piece::kBishop)));
    }
  }
}

template <MoveGenerator::Type type>
MoveGenerator::PseudoLegalMoves MoveGenerator::GeneratePseudoLegalMoves(
    Position& position) const {
  PseudoLegalMoves moves;
  const auto us = position.GetSideToMove();
  const auto them = Flip(us);
  auto target = ~position.GetPieces(us);
  if constexpr (type == Type::kCaptures || type == Type::kQuiescence) {
    target = position.GetPieces(them);
  } else if constexpr (type == Type::kQuiets) {
    target = ~position.GetAllPieces();
  }
  const auto king_square = position.GetKingSquare(us);
  const auto checkers =
      position.Attackers(king_square) & position.GetPieces(them);
  const auto king_target = target;
  if (!checkers.MoreThanOne()) {
    auto pawn_target = target;
    if constexpr (type == Type::kCaptures || type == Type::kQuiescence) {
      pawn_target |= kRankBB[0] | kRankBB[7];
    } else if constexpr (type == Type::kQuiets) {
      pawn_target &= ~(kRankBB[0] | kRankBB[7]);
    }
    if (checkers.Any()) {
      const auto attacker = checkers.GetFirstBit();
      const auto ray = Between(king_square, attacker) | SingleSquare(attacker);
      target &= ray;
      pawn_target &= ray;
    }
    GenerateMovesForPiece<Piece::kPawn>(moves, position,
                                        pawn_target & ~position.GetPieces(us));
    erase_if(moves, [&](const PseudoLegalMove move) {
      if (!move.IsEnPassant()) return false;
      if constexpr (type == Type::kQuiets) return true;
      if (checkers.None()) return false;
      const auto captured =
          static_cast<BitIndex>(move.To() + (us == Player::kWhite ? -8 : 8));
      return !checkers.Test(captured) &&
             !Between(king_square, checkers.GetFirstBit()).Test(move.To());
    });
    auto generate = [&]<Piece piece>() {
      GenerateMovesForPiece<piece>(moves, position, target);
    };
    generate.template operator()<Piece::kQueen>();
    generate.template operator()<Piece::kRook>();
    generate.template operator()<Piece::kBishop>();
    generate.template operator()<Piece::kKnight>();
  }
  GenerateMovesForPiece<Piece::kKing>(moves, position, king_target);
  if constexpr (type != Type::kCaptures) {
    GenerateCastling(moves, position);
  }
  return moves;
}

template <MoveGenerator::Type type>
MoveGenerator::LegalMoves MoveGenerator::GenerateLegalMoves(
    Position& position) const {
  const auto pseudo_legal = GeneratePseudoLegalMoves<type>(position);
  position.ComputePins(position.GetSideToMove());
  LegalMoves legal;
  for (const auto move : pseudo_legal) {
    if (position.IsLegal(move)) {
      legal.unchecked_push_back(LegalMove(static_cast<Move>(move)));
    }
  }
  return legal;
}

bool MoveGenerator::HasLegalMove(Position& position) const {
  const auto pseudo_legal = GeneratePseudoLegalMoves<Type::kAll>(position);
  position.ComputePins(position.GetSideToMove());
  return std::ranges::any_of(pseudo_legal,
                             [&position](const PseudoLegalMove move) {
                               return position.IsLegal(move);
                             });
}

template MoveGenerator::PseudoLegalMoves
MoveGenerator::GeneratePseudoLegalMoves<MoveGenerator::Type::kAll>(
    Position&) const;
template MoveGenerator::PseudoLegalMoves
MoveGenerator::GeneratePseudoLegalMoves<MoveGenerator::Type::kCaptures>(
    Position&) const;
template MoveGenerator::PseudoLegalMoves
MoveGenerator::GeneratePseudoLegalMoves<MoveGenerator::Type::kQuiets>(
    Position&) const;
template MoveGenerator::PseudoLegalMoves
MoveGenerator::GeneratePseudoLegalMoves<MoveGenerator::Type::kQuiescence>(
    Position&) const;

template <>
void MoveGenerator::GenerateMovesForPiece<Piece::kKing>(PseudoLegalMoves& moves,
                                                        Position& position,
                                                        Bitboard target) const {
  GenerateMovesFromSquare<Piece::kKing>(
      moves, position, position.GetKingSquare(position.GetSideToMove()),
      target);
}

template <Piece piece>
void MoveGenerator::GenerateMovesFromSquare(PseudoLegalMoves& moves,
                                            Position& position,
                                            const BitIndex from,
                                            Bitboard target) const {
  assert(position.GetPieceAt(from) == piece);

  // get all squares that piece attacks
  const auto attacks =
      AttackTable<piece>::GetAttackMap(from, position.GetAllPieces());

  // we move only in target squares
  auto valid_moves = attacks & target;

  while (valid_moves.Any()) {
    const auto to = valid_moves.PopFirstBit();
    moves.unchecked_push_back(PseudoLegalMove(Move(from, to)));
  }
}
}  // namespace SimpleChessEngine
