#include "MoveGenerator.h"

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

constexpr MoveGenerator::Type operator|(MoveGenerator::Type a,
                                        MoveGenerator::Type b) {
  return static_cast<MoveGenerator::Type>(static_cast<uint8_t>(a) |
                                          static_cast<uint8_t>(b));
}

constexpr MoveGenerator::Type operator&(MoveGenerator::Type a,
                                        MoveGenerator::Type b) {
  return static_cast<MoveGenerator::Type>(static_cast<uint8_t>(a) &
                                          static_cast<uint8_t>(b));
}

constexpr bool operator!(MoveGenerator::Type t) {
  return static_cast<uint8_t>(t) == 0;
}

MoveGenerator::~MoveGenerator() = default;

[[nodiscard]] bool MoveGenerator::IsPawnMoveLegal(Position& position,
                                                  const Move& move) {
  const auto us = position.GetSideToMove();

  if (std::holds_alternative<EnCroissant>(move)) {
    const auto irreversible_data = position.GetIrreversibleData();
    position.DoMove(move);
    const auto valid = !position.IsUnderCheck(us);
    position.UndoMove(move, irreversible_data);
    return valid;
  }

  BitIndex from{};
  BitIndex to{};
  std::visit(
      [&from, &to]<typename MoveType>(const MoveType& unwrapped_move) {
        if constexpr (std::same_as<std::remove_cvref_t<MoveType>,
                                   DefaultMove> ||
                      std::same_as<std::remove_cvref_t<MoveType>, PawnPush> ||
                      std::same_as<std::remove_cvref_t<MoveType>, DoublePush> ||
                      std::same_as<std::remove_cvref_t<MoveType>, Promotion>) {
          from = unwrapped_move.from;
          to = unwrapped_move.to;
          return;
        }
        assert(false);
      },
      move);
  return !position.GetIrreversibleData().blockers[static_cast<size_t>(us)].Test(
             from) ||
         Ray(position.GetKingSquare(us), from).Test(to);
}

void MoveGenerator::GenerateCastling(Moves& moves, const Position& position) {
  if (position.IsUnderCheck()) {
    return;
  }

  const auto side_to_move = position.GetSideToMove();

  const auto king_square = position.GetKingSquare(side_to_move);

  for (const auto castling_side :
       {Castling::CastlingSide::k00, Castling::CastlingSide::k000}) {
    if (position.CanCastle(castling_side)) {
      const auto rook_square =
          position.GetCastlingRookSquare(side_to_move, castling_side);
      moves.emplace_back(Castling{castling_side, king_square, rook_square});
    }
  }
}

template <Piece piece>
void MoveGenerator::GenerateMovesForPiece(Moves& moves, Position& position,
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
    Moves& moves, Position& position, const Bitboard target) const {
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

    moves.emplace_back(PawnPush{from, to});
  }

  auto double_push = Shift(double_push_pawns, direction) & valid_squares;

  double_push &= target;
  while (double_push.Any()) {
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
       ++attack_direction) {
    auto attack_squares = attacks_to[attack_direction] & target & enemy_pieces;

    while (attack_squares.Any()) {
      const auto to = attack_squares.PopFirstBit();

      const auto from = Shift(to, opposite_attacks[attack_direction]);

      moves.emplace_back(DefaultMove{from, to, position.GetPiece(to)});
    }
  }

  if (en_croissant_square) {
    const auto en_croissant_bitboard =
        GetBitboardOfSquare(en_croissant_square.value());
    for (size_t attack_direction = 0; attack_direction < attacks.size();
         ++attack_direction) {
      auto attack_to = attacks_to[attack_direction] & en_croissant_bitboard;
      if (attack_to.Any()) {
        const auto to = en_croissant_square.value();
        moves.emplace_back(
            EnCroissant{Shift(to, opposite_attacks[attack_direction]), to});
      }
    }
  }

  const auto promoting_pawns = pawns & promotion_rank;

  auto promotion_push =
      Shift(promoting_pawns, direction) & valid_squares & target;

  while (promotion_push.Any()) {
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
       ++attack_direction) {
    auto attack_squares =
        Shift(promoting_pawns & ~cant_attack_files[attack_direction],
              attacks[attack_direction]) &
        target & enemy_pieces;

    while (attack_squares.Any()) {
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

template <MoveGenerator::Type type>
MoveGenerator::Moves MoveGenerator::GenerateMoves(Position& position) const {
  moves_.clear();

  const auto us = position.GetSideToMove();
  const auto them = Flip(us);

  auto target = ~position.GetPieces(us);

  if constexpr (!!(type & Type::kQuiescence)) {
    target &= position.GetPieces(Flip(us));
  }

  const auto king_square = position.GetKingSquare(us);
  const auto king_attacker =
      position.Attackers(king_square) & position.GetPieces(them);

  // Double-check check
  if (king_attacker.MoreThanOne()) {
    GenerateMovesForPiece<Piece::kKing>(moves_, position, target);
    return moves_;
  }

  // compute pins
  position.ComputePins(us);

  const auto king_target = target;
  auto pawn_target = target;

  if constexpr (!!(type & Type::kQuiescence)) {
    pawn_target |= (kRankBB[0] | kRankBB[7]);
  }
  if constexpr (!!(type & Type::kAddChecks)) {
    pawn_target |= GetPawnAttacks(king_square, us);
  }
  // is in check
  if (king_attacker.Any()) {
    const auto attacker = king_attacker.GetFirstBit();
    const auto ray =
        Between(king_square, attacker) | GetBitboardOfSquare(attacker);
    target &= ray;
    pawn_target &= ray;
  }

  GenerateMovesForPiece<Piece::kPawn>(moves_, position,
                                      pawn_target & ~position.GetPieces(us));

  std::erase_if(moves_, [&position](const Move& move) {
    return !IsPawnMoveLegal(position, move);
  });

  // generate moves for piece
  auto generate_move_for_piece = [this, &position, king_square,
                                  us]<Piece piece>(Bitboard target) {
    if constexpr (!!(type & Type::kAddChecks)) {
      target |=
          AttackTable<piece>::GetAttackMap(king_square, position.GetPieces(us));
    }
    GenerateMovesForPiece<piece>(moves_, position,
                                 target & ~position.GetPieces(us));
  };

  auto generate_moves = [target, &generate_move_for_piece]<Piece... pieces>() {
    (generate_move_for_piece.template operator()<pieces>(target), ...);
  };

  generate_moves.template
  operator()<Piece::kQueen, Piece::kRook, Piece::kBishop, Piece::kKnight>();

  GenerateMovesForPiece<Piece::kKing>(moves_, position,
                                      king_target & ~position.GetPieces(us));
  GenerateCastling(moves_, position);

  // return moves
  return moves_;
}

template <>
void MoveGenerator::GenerateMovesForPiece<Piece::kKing>(Moves& moves,
                                                        Position& position,
                                                        Bitboard target) const {
  const auto us = position.GetSideToMove();
  const auto them = Flip(us);

  const auto king_pos = position.GetKingSquare(us);
  const auto king_mask = GetBitboardOfSquare(king_pos);

  const auto occupancy = position.GetAllPieces() ^ king_mask;

  // we prevent the king from going to squares attacked by enemy pieces

  target &= ~position.GetAllPawnAttacks(Flip(us));

  Bitboard attackers = position.GetPiecesByType<Piece::kKnight>(them);
  while (attackers.Any()) {
    target &= ~AttackTable<Piece::kKnight>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kBishop>(them);
  while (attackers.Any()) {
    target &= ~AttackTable<Piece::kBishop>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kRook>(them);
  while (attackers.Any()) {
    target &= ~AttackTable<Piece::kRook>::GetAttackMap(attackers.PopFirstBit(),
                                                       occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kQueen>(them);
  while (attackers.Any()) {
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
                                            Bitboard target) const {
  assert(position.GetPiece(from) == piece);

  // get all squares that piece attacks
  const auto attacks =
      AttackTable<piece>::GetAttackMap(from, position.GetAllPieces());

  // get whose move is now
  const auto side_to_move = position.GetSideToMove();

  // if the piece is pinned we can only move in pin direction
  if (position.GetIrreversibleData()
          .blockers[static_cast<size_t>(side_to_move)]
          .Test(from)) {
    target &= Ray(position.GetKingSquare(side_to_move), from);
  }

  // we move only in target squares
  auto valid_moves = attacks & target;

  while (valid_moves.Any()) {
    const auto to = valid_moves.PopFirstBit();

    const auto move = DefaultMove{from, to, position.GetPiece(to)};
    moves.emplace_back(move);
  }
}
}  // namespace SimpleChessEngine