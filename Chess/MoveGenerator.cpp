#include "MoveGenerator.h"

#include <cassert>

#include "Attacks.h"
#include "BitBoard.h"

namespace SimpleChessEngine {

template MoveList<PseudoLegalTag> MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kCaptures>(const Position& position) const;
template MoveList<PseudoLegalTag> MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kQuiets>(const Position& position) const;
template MoveList<PseudoLegalTag> MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kEvasions>(const Position& position) const;
template MoveList<PseudoLegalTag> MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kNonEvasions>(const Position& position) const;
template MoveList<LegalTag> MoveGenerator::GenerateMoves<
    MoveGenerator::Type::kLegal>(const Position& position) const;

MoveGenerator::~MoveGenerator() = default;

template <Player Us, MoveGenerator::Type GenType>
void MoveGenerator::GeneratePawnMoves(Moves& moves, const Position& position,
                                      Bitboard target) const {
  constexpr Player Them = Flip(Us);
  constexpr Bitboard promotion_rank =
      (Us == Player::kWhite ? kRankBB[6] : kRankBB[1]);
  constexpr Bitboard third_rank =
      (Us == Player::kWhite ? kRankBB[2] : kRankBB[5]);
  constexpr Compass up = kPawnMoveDirection[static_cast<size_t>(Us)];
  constexpr Compass down = kPawnMoveDirection[static_cast<size_t>(Them)];

  const auto pawns = position.GetPiecesByType<Piece::kPawn>(Us);
  const auto empty_squares = ~position.GetAllPieces();
  const auto enemies = GenType == Type::kEvasions
                           ? (position.Attackers(position.GetKingSquare(Us)) &
                              position.GetPieces(Them))
                           : position.GetPieces(Them);

  const auto pawns_on_7 = pawns & promotion_rank;
  const auto pawns_not_on_7 = pawns & ~promotion_rank;

  if constexpr (GenType != Type::kCaptures) {
    auto push = Shift(pawns_not_on_7, up) & empty_squares;
    auto double_push = Shift(push & third_rank, up) & empty_squares;

    if constexpr (GenType == Type::kEvasions) {
      push &= target;
      double_push &= target;
    }

    while (push.Any()) {
      const auto to = push.PopFirstBit();
      const auto from = Shift(to, down);
      moves.emplace_back(Move(from, to));
    }

    while (double_push.Any()) {
      const auto to = double_push.PopFirstBit();
      const auto from = Shift(Shift(to, down), down);
      moves.emplace_back(Move(from, to));
    }
  }

  if (pawns_on_7.Any()) {
    constexpr std::array attack_dirs = {
        (Us == Player::kWhite ? Compass::kNorthWest : Compass::kSouthWest),
        (Us == Player::kWhite ? Compass::kNorthEast : Compass::kSouthEast)};
    constexpr std::array opposite_dirs = {
        (Us == Player::kWhite ? Compass::kSouthEast : Compass::kNorthEast),
        (Us == Player::kWhite ? Compass::kSouthWest : Compass::kNorthWest)};

    auto promo_push = Shift(pawns_on_7, up) & empty_squares;
    if constexpr (GenType == Type::kEvasions) {
      promo_push &= target;
    }

    while (promo_push.Any()) {
      const auto to = promo_push.PopFirstBit();
      const auto from = Shift(to, down);
      if constexpr (GenType == Type::kCaptures || GenType == Type::kEvasions ||
                    GenType == Type::kNonEvasions) {
        moves.emplace_back(
            Move::Make<MoveType::kPromotion>(from, to, Piece::kQueen));
      }
      if constexpr (GenType == Type::kQuiets || GenType == Type::kEvasions ||
                    GenType == Type::kNonEvasions) {
        moves.emplace_back(
            Move::Make<MoveType::kPromotion>(from, to, Piece::kRook));
        moves.emplace_back(
            Move::Make<MoveType::kPromotion>(from, to, Piece::kBishop));
        moves.emplace_back(
            Move::Make<MoveType::kPromotion>(from, to, Piece::kKnight));
      }
    }

    for (size_t i = 0; i < 2; ++i) {
      auto promo_capture =
          Shift(pawns_on_7 & ~(i == 0 ? kFileBB[0] : kFileBB[7]),
                attack_dirs[i]) &
          enemies;
      while (promo_capture.Any()) {
        const auto to = promo_capture.PopFirstBit();
        const auto from = Shift(to, opposite_dirs[i]);
        if constexpr (GenType == Type::kCaptures ||
                      GenType == Type::kEvasions ||
                      GenType == Type::kNonEvasions) {
          moves.emplace_back(
              Move::Make<MoveType::kPromotion>(from, to, Piece::kQueen));
        }
        if constexpr (GenType == Type::kQuiets || GenType == Type::kEvasions ||
                      GenType == Type::kNonEvasions) {
          moves.emplace_back(
              Move::Make<MoveType::kPromotion>(from, to, Piece::kRook));
          moves.emplace_back(
              Move::Make<MoveType::kPromotion>(from, to, Piece::kBishop));
          moves.emplace_back(
              Move::Make<MoveType::kPromotion>(from, to, Piece::kKnight));
        }
      }
    }
  }

  if constexpr (GenType == Type::kCaptures || GenType == Type::kEvasions ||
                GenType == Type::kNonEvasions) {
    constexpr std::array attack_dirs = {
        (Us == Player::kWhite ? Compass::kNorthWest : Compass::kSouthWest),
        (Us == Player::kWhite ? Compass::kNorthEast : Compass::kSouthEast)};
    constexpr std::array opposite_dirs = {
        (Us == Player::kWhite ? Compass::kSouthEast : Compass::kNorthEast),
        (Us == Player::kWhite ? Compass::kSouthWest : Compass::kNorthWest)};

    for (size_t i = 0; i < 2; ++i) {
      auto captures =
          Shift(pawns_not_on_7 & ~(i == 0 ? kFileBB[0] : kFileBB[7]),
                attack_dirs[i]) &
          enemies;
      while (captures.Any()) {
        const auto to = captures.PopFirstBit();
        const auto from = Shift(to, opposite_dirs[i]);
        moves.emplace_back(Move(from, to));
      }
    }

    if (const auto ep_square = position.GetEnCroissantSquare();
        ep_square.has_value()) {
      if constexpr (GenType == Type::kEvasions) {
        if ((target & SingleSquare(Shift(ep_square.value(), down))).None()) {
          return;
        }
      }

      const auto ep_bb = SingleSquare(ep_square.value());
      for (size_t i = 0; i < 2; ++i) {
        auto ep_capture =
            Shift(pawns_not_on_7 & ~(i == 0 ? kFileBB[0] : kFileBB[7]),
                  attack_dirs[i]) &
            ep_bb;
        if (ep_capture.Any()) {
          const auto to = ep_square.value();
          const auto from = Shift(to, opposite_dirs[i]);
          moves.emplace_back(Move::Make<MoveType::kEnPassant>(from, to));
        }
      }
    }
  }
}

template <Player Us, Piece Pt>
void MoveGenerator::GeneratePieceMoves(Moves& moves, const Position& position,
                                       Bitboard target) const {
  static_assert(Pt != Piece::kKing && Pt != Piece::kPawn);

  auto pieces = position.GetPiecesByType<Pt>(Us);
  while (pieces.Any()) {
    const auto from = pieces.PopFirstBit();
    auto attacks =
        AttackTable<Pt>::GetAttackMap(from, position.GetAllPieces()) & target;

    if (position.GetIrreversibleData().blockers[static_cast<size_t>(Us)].Test(
            from)) {
      attacks &= Ray(position.GetKingSquare(Us), from);
    }

    while (attacks.Any()) {
      const auto to = attacks.PopFirstBit();
      moves.emplace_back(Move(from, to));
    }
  }
}

void MoveGenerator::GenerateCastling(Moves& moves, const Position& position,
                                     Player us) const {
  if (position.IsUnderCheck(us)) {
    return;
  }

  const auto king_square = position.GetKingSquare(us);
  const auto color_idx = static_cast<size_t>(us);

  for (const auto castling_side : {CastlingSide::k00, CastlingSide::k000}) {
    if (position.CanCastle(castling_side)) {
      const auto side_idx = static_cast<size_t>(castling_side);
      const auto king_to = kKingCastlingDestination[color_idx][side_idx];
      moves.emplace_back(Move::Make<MoveType::kCastling>(king_square, king_to));
    }
  }
}

template <Player Us, MoveGenerator::Type GenType>
void MoveGenerator::GenerateAll(Moves& moves, const Position& position) const {
  const auto king_square = position.GetKingSquare(Us);
  const auto them = Flip(Us);

  Bitboard target;

  const auto checkers =
      position.Attackers(king_square) & position.GetPieces(them);

  if constexpr (GenType != Type::kEvasions) {
    if (checkers.Any()) {
      return;
    }
  }

  if (checkers.MoreThanOne()) {
    target = GenType == Type::kEvasions ? ~position.GetPieces(Us) : target;
    auto king_attacks = AttackTable<Piece::kKing>::GetAttackMap(
                            king_square, position.GetAllPieces()) &
                        target;

    const auto occupancy = position.GetAllPieces() ^ SingleSquare(king_square);
    king_attacks &= ~position.GetAllPawnAttacks(them);

    auto attackers = position.GetPiecesByType<Piece::kKnight>(them);
    while (attackers.Any()) {
      king_attacks &= ~AttackTable<Piece::kKnight>::GetAttackMap(
          attackers.PopFirstBit(), occupancy);
    }

    attackers = position.GetPiecesByType<Piece::kBishop>(them);
    while (attackers.Any()) {
      king_attacks &= ~AttackTable<Piece::kBishop>::GetAttackMap(
          attackers.PopFirstBit(), occupancy);
    }

    attackers = position.GetPiecesByType<Piece::kRook>(them);
    while (attackers.Any()) {
      king_attacks &= ~AttackTable<Piece::kRook>::GetAttackMap(
          attackers.PopFirstBit(), occupancy);
    }

    attackers = position.GetPiecesByType<Piece::kQueen>(them);
    while (attackers.Any()) {
      king_attacks &= ~AttackTable<Piece::kQueen>::GetAttackMap(
          attackers.PopFirstBit(), occupancy);
    }

    king_attacks &= ~AttackTable<Piece::kKing>::GetAttackMap(
        position.GetKingSquare(them), occupancy);

    while (king_attacks.Any()) {
      const auto to = king_attacks.PopFirstBit();
      moves.emplace_back(Move(king_square, to));
    }
    return;
  }

  if constexpr (GenType == Type::kEvasions) {
    target = Between(king_square, checkers.GetFirstBit()) | checkers;
  } else if constexpr (GenType == Type::kNonEvasions) {
    target = ~position.GetPieces(Us);
  } else if constexpr (GenType == Type::kCaptures) {
    target = position.GetPieces(them);
  } else {
    target = ~position.GetAllPieces();
  }

  GeneratePawnMoves<Us, GenType>(moves, position, target);
  GeneratePieceMoves<Us, Piece::kKnight>(moves, position, target);
  GeneratePieceMoves<Us, Piece::kBishop>(moves, position, target);
  GeneratePieceMoves<Us, Piece::kRook>(moves, position, target);
  GeneratePieceMoves<Us, Piece::kQueen>(moves, position, target);

  auto king_target =
      GenType == Type::kEvasions ? ~position.GetPieces(Us) : target;
  auto king_attacks = AttackTable<Piece::kKing>::GetAttackMap(
                          king_square, position.GetAllPieces()) &
                      king_target;

  const auto occupancy = position.GetAllPieces() ^ SingleSquare(king_square);
  king_attacks &= ~position.GetAllPawnAttacks(them);

  auto attackers = position.GetPiecesByType<Piece::kKnight>(them);
  while (attackers.Any()) {
    king_attacks &= ~AttackTable<Piece::kKnight>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kBishop>(them);
  while (attackers.Any()) {
    king_attacks &= ~AttackTable<Piece::kBishop>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kRook>(them);
  while (attackers.Any()) {
    king_attacks &= ~AttackTable<Piece::kRook>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  attackers = position.GetPiecesByType<Piece::kQueen>(them);
  while (attackers.Any()) {
    king_attacks &= ~AttackTable<Piece::kQueen>::GetAttackMap(
        attackers.PopFirstBit(), occupancy);
  }

  king_attacks &= ~AttackTable<Piece::kKing>::GetAttackMap(
      position.GetKingSquare(them), occupancy);

  while (king_attacks.Any()) {
    const auto to = king_attacks.PopFirstBit();
    moves.emplace_back(Move(king_square, to));
  }

  if constexpr (GenType == Type::kQuiets || GenType == Type::kNonEvasions) {
    GenerateCastling(moves, position, Us);
  }
}

template <MoveGenerator::Type type>
auto MoveGenerator::GenerateMoves(const Position& position) const
    -> std::conditional_t<type == Type::kLegal, MoveList<LegalTag>,
                          MoveList<PseudoLegalTag>> {
  moves_.clear();

  const auto us = position.GetSideToMove();
  position.ComputePins(us);  // TODO: OPTIMIZE, dont need to call it always

  if constexpr (type == Type::kLegal) {
    const auto pinned =
        position.GetIrreversibleData().blockers[static_cast<size_t>(us)] &
        position.GetPieces(us);
    const auto king_square = position.GetKingSquare(us);

    const size_t initial_size = moves_.size();

    if (position.IsUnderCheck(us)) {
      if (us == Player::kWhite) {
        GenerateAll<Player::kWhite, Type::kEvasions>(moves_, position);
      } else {
        GenerateAll<Player::kBlack, Type::kEvasions>(moves_, position);
      }
    } else {
      if (us == Player::kWhite) {
        GenerateAll<Player::kWhite, Type::kNonEvasions>(moves_, position);
      } else {
        GenerateAll<Player::kBlack, Type::kNonEvasions>(moves_, position);
      }
    }

    auto it = moves_.begin() + initial_size;
    while (it != moves_.end()) {
      if (((pinned.Test(it->From())) || it->From() == king_square ||
           it->IsEnPassant()) &&
          !position.Legal(*it)) {
        *it = moves_.back();
        moves_.pop_back();
      } else {
        ++it;
      }
    }

    return MoveList<LegalTag>(std::move(moves_));
  } else {
    if (us == Player::kWhite) {
      GenerateAll<Player::kWhite, type>(moves_, position);
    } else {
      GenerateAll<Player::kBlack, type>(moves_, position);
    }

    return MoveList<PseudoLegalTag>(std::move(moves_));
  }
}

}  // namespace SimpleChessEngine
