#include "MoveGenerator.h"

#include <algorithm>
#include <cassert>

#include "Attacks.h"
#include "BitBoard.h"

namespace SimpleChessEngine {
template MoveGenerator::Moves MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kAll>(Position& position) const;
template MoveGenerator::Moves MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kQuiescence>(Position& position) const;
template MoveGenerator::Moves MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kAddChecks>(Position& position) const;

MoveGenerator::~MoveGenerator() = default;

void MoveGenerator::GenerateCastling(Move*& moves, const Position& position) {
  if (position.IsUnderCheck()) {
    return;
  }

  const auto side_to_move = position.GetSideToMove();
  const auto king_square = position.GetKingSquare(side_to_move);
  const auto color_idx = static_cast<size_t>(side_to_move);

  for (const auto castling_side : {CastlingSide::k00, CastlingSide::k000}) {
    if (position.CanPseudoCastle(castling_side)) {
      const auto side_idx = static_cast<size_t>(castling_side);
      const auto king_to = kKingCastlingDestination[color_idx][side_idx];

      *moves++ = Move::Make<MoveType::kCastling>(king_square, king_to);
    }
  }
}

template <Piece piece>
void MoveGenerator::GenerateMovesForPiece(Move*& moves, Position& position,
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
    Move*& moves, Position& position, const Bitboard target) const {
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
  while (push.Any()) {
    const auto to = push.PopFirstBit();
    const auto from = Shift(to, opposite_direction);
    *moves++ = Move(from, to);
  }

  auto double_push = Shift(double_push_pawns, direction) & valid_squares;

  double_push &= target;
  while (double_push.Any()) {
    const auto to = double_push.PopFirstBit();
    const auto from = Shift(Shift(to, opposite_direction), opposite_direction);
    *moves++ = Move(from, to);
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
       ++attack_direction) {
    auto attack_squares = attacks_to[attack_direction] & target & enemy_pieces;

    while (attack_squares.Any()) {
      const auto to = attack_squares.PopFirstBit();
      const auto from = Shift(to, opposite_attacks[attack_direction]);
      *moves++ = Move(from, to);
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
        *moves++ = Move::Make<MoveType::kEnPassant>(from, to);
      }
    }
  }

  const auto promoting_pawns = pawns & promotion_rank;

  auto promotion_push =
      Shift(promoting_pawns, direction) & valid_squares & target;

  while (promotion_push.Any()) {
    const auto to = promotion_push.PopFirstBit();
    const auto from = Shift(to, opposite_direction);

    *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kQueen);
    *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kKnight);
    *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kRook);
    *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kBishop);
  }

  for (size_t attack_direction = 0; attack_direction < attacks.size();
       ++attack_direction) {
    auto attack_squares =
        Shift(promoting_pawns & ~cant_attack_files[attack_direction],
              attacks[attack_direction]) &
        target & enemy_pieces;

    while (attack_squares.Any()) {
      const auto to = attack_squares.PopFirstBit();
      const auto from = Shift(to, opposite_attacks[attack_direction]);

      *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kQueen);
      *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kKnight);
      *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kRook);
      *moves++ = Move::Make<MoveType::kPromotion>(from, to, Piece::kBishop);
    }
  }
}

template <MoveGenerator::Type type>
Move* MoveGenerator::GeneratePseudoLegal(Position& position,
                                         Move* moves) const {
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
    if constexpr (type == Type::kAddChecks) {
      pawn_target |= GetPawnAttacks(king_square, us);
    }
    if (checkers.Any()) {
      const auto attacker = checkers.GetFirstBit();
      const auto ray = Between(king_square, attacker) | SingleSquare(attacker);
      target &= ray;
      pawn_target &= ray;
    }
    auto* pawn_begin = moves;
    GenerateMovesForPiece<Piece::kPawn>(moves, position,
                                        pawn_target & ~position.GetPieces(us));
    auto* pawn_end = moves;
    moves = pawn_begin;
    for (auto* p = pawn_begin; p != pawn_end; ++p) {
      if constexpr (type == Type::kQuiets) {
        if (p->IsEnPassant()) continue;
      }
      if (p->IsEnPassant() && checkers.Any()) {
        const auto captured =
            static_cast<BitIndex>(p->To() + (us == Player::kWhite ? -8 : 8));
        if (!checkers.Test(captured) &&
            !Between(king_square, checkers.GetFirstBit()).Test(p->To()))
          continue;
      }
      *moves++ = *p;
    }
    auto generate = [&]<Piece piece>() {
      auto piece_target = target;
      if constexpr (type == Type::kAddChecks) {
        piece_target |= AttackTable<piece>::GetAttackMap(
            king_square, position.GetPieces(us));
      }
      GenerateMovesForPiece<piece>(moves, position,
                                   piece_target & ~position.GetPieces(us));
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
MoveGenerator::Moves MoveGenerator::GenerateMoves(Position& position) const {
  std::array<Move, kMaxMovesPerPosition> moves;
  auto* end = GeneratePseudoLegal<type>(position, moves.data());
  position.ComputePins(position.GetSideToMove());
  Moves legal;
  legal.reserve(end - moves.data());
  for (auto* p = moves.data(); p != end; ++p) {
    if (position.IsLegal(*p)) legal.push_back(*p);
  }
  return legal;
}

bool MoveGenerator::HasLegalMove(Position& position) const {
  std::array<Move, kMaxMovesPerPosition> moves;
  auto* end = GeneratePseudoLegal<Type::kAll>(position, moves.data());
  position.ComputePins(position.GetSideToMove());
  return std::any_of(moves.data(), end,
                     [&position](Move move) { return position.IsLegal(move); });
}

template Move* MoveGenerator::GeneratePseudoLegal<MoveGenerator::Type::kAll>(
    Position&, Move*) const;
template Move*
MoveGenerator::GeneratePseudoLegal<MoveGenerator::Type::kCaptures>(Position&,
                                                                   Move*) const;
template Move* MoveGenerator::GeneratePseudoLegal<MoveGenerator::Type::kQuiets>(
    Position&, Move*) const;
template Move* MoveGenerator::GeneratePseudoLegal<
    MoveGenerator::Type::kQuiescence>(Position&, Move*) const;

template <>
void MoveGenerator::GenerateMovesForPiece<Piece::kKing>(Move*& moves,
                                                        Position& position,
                                                        Bitboard target) const {
  GenerateMovesFromSquare<Piece::kKing>(
      moves, position, position.GetKingSquare(position.GetSideToMove()),
      target);
}

template <Piece piece>
void MoveGenerator::GenerateMovesFromSquare(Move*& moves, Position& position,
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
    *moves++ = Move(from, to);
  }
}
}  // namespace SimpleChessEngine
