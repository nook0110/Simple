#include "Position.h"

#include <numeric>

#include "Attacks.h"
#include "BitBoard.h"
#include "PSQT.h"
#include "Piece.h"

namespace SimpleChessEngine {

template Bitboard Position::GetPiecesByType<Piece::kPawn>(Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kKnight>(
    Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kBishop>(
    Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kRook>(Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kQueen>(Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kKing>(Player player) const;

const Hasher Position::hasher_{std::mt19937_64(0xb00b1e5)};

/**
 * \brief Places a piece with a color on a chosen square.
 *
 * \param square Square to place piece.
 * \param piece Piece to place.
 * \param color Color of the piece.
 *
 */
void Position::PlacePiece(const BitIndex square, const Piece piece,
                          const Player color) {
  assert(IsOk(square));
  assert(!board_[square]);
  assert(piece);
  const auto piece_idx = static_cast<size_t>(piece);
  const auto color_idx = static_cast<size_t>(color);
  board_[square] = piece;
  pieces_by_type_[piece_idx].Set(square);
  pieces_by_color_[color_idx].Set(square);
  ++evaluation_data_.piece_counts[color_idx][piece_idx];
  evaluation_data_.material[color_idx] += kPieceValues[piece_idx];
  evaluation_data_.psqt[color_idx] += kPSQT[color_idx][piece_idx][square];
  if (piece != Piece::kPawn)
    evaluation_data_.non_pawn_material += kPieceValues[piece_idx].eval[0];
  hash_ ^= hasher_.psqt_hash[piece_idx][color_idx][square];
}

/**
 * \brief Removes a piece from a chosen square.
 *
 * \param square Square to remove piece from.
 * \param color Color of the piece.
 *
 */

void Position::RemovePiece(const BitIndex square, const Player color) {
  assert(IsOk(square));
  const auto piece = board_[square];
  assert(!!piece);
  const auto piece_idx = static_cast<size_t>(piece);
  const auto color_idx = static_cast<size_t>(color);
  pieces_by_type_[piece_idx].Reset(square);
  pieces_by_color_[color_idx].Reset(square);
  --evaluation_data_.piece_counts[color_idx][piece_idx];
  board_[square] = Piece::kNone;
  evaluation_data_.material[color_idx] -= kPieceValues[piece_idx];
  evaluation_data_.psqt[color_idx] -= kPSQT[color_idx][piece_idx][square];
  if (piece != Piece::kPawn)
    evaluation_data_.non_pawn_material -= kPieceValues[piece_idx].eval[0];
  hash_ ^= hasher_.psqt_hash[piece_idx][color_idx][square];
}

void Position::MovePiece(const BitIndex from, const BitIndex to,
                         const Player color) {
  assert(IsOk(from));
  assert(IsOk(to));
  const auto piece = board_[from];
  assert(!!piece);
  assert(!board_[to]);
  const auto piece_idx = static_cast<size_t>(piece);
  const auto color_idx = static_cast<size_t>(color);
  pieces_by_type_[piece_idx].Reset(from);
  pieces_by_color_[color_idx].Reset(from);
  pieces_by_type_[piece_idx].Set(to);
  pieces_by_color_[color_idx].Set(to);
  board_[from] = Piece::kNone;
  board_[to] = piece;
  evaluation_data_.psqt[color_idx] -= kPSQT[color_idx][piece_idx][from];
  evaluation_data_.psqt[color_idx] += kPSQT[color_idx][piece_idx][to];
  hash_ ^= hasher_.psqt_hash[piece_idx][color_idx][from];
  hash_ ^= hasher_.psqt_hash[piece_idx][color_idx][to];
}

void Position::DoMove(const Move &move) {
  if (const auto &ep_square = irreversible_data_.en_croissant_square;
      ep_square.has_value()) {
    hash_ ^= hasher_.en_croissant_hash[GetCoordinates(ep_square.value()).first];
    irreversible_data_.en_croissant_square.reset();
  }
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= hasher_.cr_hash[static_cast<size_t>(
        color)][irreversible_data_.castling_rights[static_cast<size_t>(color)]
                    .to_ulong()];
  }

  const auto from = move.From();
  const auto to = move.To();
  const auto us = side_to_move_;
  const auto them = Flip(us);
  
  // Store captured piece for undo
  irreversible_data_.captured_piece = board_[to];

  if (move.IsEnPassant()) {
    const auto capture_square = Shift(to, kPawnMoveDirection[static_cast<size_t>(them)]);
    irreversible_data_.captured_piece = Piece::kPawn;  // En passant always captures a pawn
    RemovePiece(capture_square, them);
    MovePiece(from, to, us);
  } else if (move.IsPromotion()) {
    const auto promoted_to = move.PromotionPiece();
    const auto captured_piece = board_[to];
    RemovePiece(from, us);
    if (!!captured_piece) RemovePiece(to, them);
    PlacePiece(to, promoted_to, us);
    
    for (const auto castling_side : {CastlingSide::k00, CastlingSide::k000}) {
      const auto their_rook = rook_positions_[static_cast<size_t>(them)][static_cast<size_t>(castling_side)];
      if (to == their_rook) {
        irreversible_data_.castling_rights[static_cast<size_t>(them)] &=
            ~static_cast<char>(kCastlingRightsForSide[static_cast<size_t>(castling_side)]);
      }
    }
  } else if (move.IsCastling()) {
    irreversible_data_.captured_piece = Piece::kNone;  // Castling never captures
    // Determine castling side and rook position from move
    const auto king_to = to;
    const auto king_from = from;
    
    // Determine which side based on king destination
    const auto color_idx = static_cast<size_t>(us);
    CastlingSide side;
    BitIndex rook_from;
    
    if (king_to == kKingCastlingDestination[color_idx][0]) {
      side = CastlingSide::k00;
      rook_from = rook_positions_[color_idx][0];
    } else {
      side = CastlingSide::k000;
      rook_from = rook_positions_[color_idx][1];
    }
    
    const auto side_idx = static_cast<size_t>(side);
    RemovePiece(king_from, us);
    RemovePiece(rook_from, us);
    PlacePiece(kKingCastlingDestination[color_idx][side_idx], Piece::kKing, us);
    PlacePiece(kRookCastlingDestination[color_idx][side_idx], Piece::kRook, us);
    
    king_position_[color_idx] = kKingCastlingDestination[color_idx][side_idx];
    irreversible_data_.castling_rights[color_idx] = 0;
  } else {
    // Normal move (including pawn pushes and double pushes)
    const auto piece_to_move = board_[from];
    const auto captured_piece = board_[to];
    
    // Check if it's a double pawn push
    if (piece_to_move == Piece::kPawn && std::abs(from - to) == 16) {
      const auto file = GetCoordinates(from).first;
      hash_ ^= hasher_.en_croissant_hash[file];
      irreversible_data_.en_croissant_square = std::midpoint(from, to);
    }
    
    if (!!captured_piece) RemovePiece(to, them);
    MovePiece(from, to, us);
    
    if (piece_to_move == Piece::kKing) {
      king_position_[static_cast<size_t>(us)] = to;
      irreversible_data_.castling_rights[static_cast<size_t>(us)] = 0;
    }
    
    for (auto castling_side : {CastlingSide::k00, CastlingSide::k000}) {
      const auto our_rook = rook_positions_[static_cast<size_t>(us)][static_cast<size_t>(castling_side)];
      const auto their_rook = rook_positions_[static_cast<size_t>(them)][static_cast<size_t>(castling_side)];
      if (from == our_rook) {
        irreversible_data_.castling_rights[static_cast<size_t>(us)] &=
            ~static_cast<std::int8_t>(kCastlingRightsForSide[static_cast<size_t>(castling_side)]);
      }
      if (to == their_rook) {
        irreversible_data_.castling_rights[static_cast<size_t>(them)] &=
            ~static_cast<std::int8_t>(kCastlingRightsForSide[static_cast<size_t>(castling_side)]);
      }
    }
  }

  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= hasher_.cr_hash[static_cast<size_t>(
        color)][irreversible_data_.castling_rights[static_cast<size_t>(color)]
                    .to_ulong()];
  }
  side_to_move_ = Flip(side_to_move_);
  hash_ ^= hasher_.stm_hash;

  // 50-move rule: reset counter if it's a pawn move or a capture
  const bool resets_fifty_move =
      (board_[to] == Piece::kPawn) ||  // Moving piece is a pawn
      (irreversible_data_.captured_piece != Piece::kNone);  // Any capture
  history_stack_.Push(hash_, resets_fifty_move);
}

void SimpleChessEngine::Position::DoMove(NullMove) {
  if (const auto &ep_square = irreversible_data_.en_croissant_square;
      ep_square.has_value()) {
    hash_ ^= hasher_.en_croissant_hash[GetCoordinates(ep_square.value()).first];
    irreversible_data_.en_croissant_square.reset();
  }
  side_to_move_ = Flip(side_to_move_);
  hash_ ^= hasher_.stm_hash;

  history_stack_.Push(hash_, false);
}


void Position::UndoMove(const Move &move, const IrreversibleData &data) {
  const auto &ep_square = irreversible_data_.en_croissant_square;
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= hasher_.cr_hash[static_cast<size_t>(
        color)][irreversible_data_.castling_rights[static_cast<size_t>(color)]
                    .to_ulong()];
  }
  if (ep_square.has_value()) {
    hash_ ^= hasher_.en_croissant_hash[GetCoordinates(ep_square.value()).first];
  }
  
  const auto from = move.From();
  const auto to = move.To();
  const auto them = side_to_move_;  // Current side (after the move was made)
  const auto us = Flip(them);       // Side that made the move
  
  const auto captured_piece = irreversible_data_.captured_piece;
  
  irreversible_data_ = data;
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= hasher_.cr_hash[static_cast<size_t>(
        color)][irreversible_data_.castling_rights[static_cast<size_t>(color)]
                    .to_ulong()];
  }
  if (irreversible_data_.en_croissant_square.has_value()) {
    hash_ ^= hasher_.en_croissant_hash[GetCoordinates(irreversible_data_.en_croissant_square.value()).first];
  }
  hash_ ^= hasher_.stm_hash;
  side_to_move_ = Flip(side_to_move_);

  if (move.IsEnPassant()) {
    const auto capture_square = Shift(to, kPawnMoveDirection[static_cast<size_t>(them)]);
    MovePiece(to, from, us);
    PlacePiece(capture_square, Piece::kPawn, them);
  } else if (move.IsPromotion()) {
    RemovePiece(to, us);
    if (!!captured_piece) PlacePiece(to, captured_piece, them);
    PlacePiece(from, Piece::kPawn, us);
  } else if (move.IsCastling()) {
    const auto color_idx = static_cast<size_t>(us);
    
    CastlingSide side;
    if (to == kKingCastlingDestination[color_idx][0]) {
      side = CastlingSide::k00;
    } else {
      side = CastlingSide::k000;
    }
    
    const auto side_idx = static_cast<size_t>(side);
    const auto rook_from = rook_positions_[color_idx][side_idx];
    
    RemovePiece(kKingCastlingDestination[color_idx][side_idx], us);
    RemovePiece(kRookCastlingDestination[color_idx][side_idx], us);
    
    PlacePiece(from, Piece::kKing, us);
    PlacePiece(rook_from, Piece::kRook, us);
    
    king_position_[color_idx] = from;
  } else {
    // Normal move
    const auto piece_to_move = board_[to];
    
    MovePiece(to, from, us);
    if (!!captured_piece) PlacePiece(to, captured_piece, them);
    
    if (piece_to_move == Piece::kKing) {
      king_position_[static_cast<size_t>(us)] = from;
    }
  }

  history_stack_.Pop();
}

void SimpleChessEngine::Position::UndoMove(NullMove,
                                           const IrreversibleData &data) {
  irreversible_data_ = data;
  const auto &ep_square = irreversible_data_.en_croissant_square;
  if (ep_square.has_value()) {
    hash_ ^= hasher_.en_croissant_hash[GetCoordinates(ep_square.value()).first];
  }
  hash_ ^= hasher_.stm_hash;
  side_to_move_ = Flip(side_to_move_);

  history_stack_.Pop();
}


[[nodiscard]] bool Position::CanCastle(
    const CastlingSide castling_side) const {
  const auto us = side_to_move_;
  const auto us_idx = static_cast<size_t>(us);
  const auto cs_idx = static_cast<size_t>(castling_side);

  if (!irreversible_data_.castling_rights[us_idx].test(cs_idx)) return false;

  const auto king_position = king_position_[us_idx];
  const auto rook_position = rook_positions_[us_idx][cs_idx];

  const auto obstacles = GetAllPieces() & ~SingleSquare(king_position) &
                         ~SingleSquare(rook_position);

  auto king_path = castling_squares_for_king_[us_idx][cs_idx];

  if (((king_path | castling_squares_for_rook_[us_idx][cs_idx]) & obstacles)
          .Any())
    return false;

  while (king_path.Any()) {
    if (const auto square = king_path.PopFirstBit(); IsUnderAttack(square, us))
      return false;
  }

  return true;
}

void Position::SetCastlingRights(
    const std::array<std::bitset<2>, 2> &castling_rights) {
  irreversible_data_.castling_rights = castling_rights;
}

void Position::SetKingPositions(const std::array<BitIndex, 2> &king_position) {
  king_position_ = king_position;
}

void Position::SetRookPositions(
    const std::array<std::array<BitIndex, 2>, kColors> &rook_positions) {
  rook_positions_ = rook_positions;
}

void Position::SetCastlingSquares(
    const std::array<std::array<Bitboard, 2>, kColors> &cs_king,
    const std::array<std::array<Bitboard, 2>, kColors> &cs_rook) {
  castling_squares_for_king_ = cs_king;
  castling_squares_for_rook_ = cs_rook;
}

Bitboard Position::GetAllPieces() const {
  return pieces_by_color_[static_cast<size_t>(Player::kWhite)] |
         pieces_by_color_[static_cast<size_t>(Player::kBlack)];
}

bool Position::IsInsufficientMaterial() const {
  constexpr Bitboard kDarkSquares{0xAA55AA55AA55AA55ULL};
  constexpr Bitboard kLightSquares{0x55AA55AA55AA55AAULL};

  if (pieces_by_type_[static_cast<size_t>(Piece::kPawn)].Any() ||
      pieces_by_type_[static_cast<size_t>(Piece::kRook)].Any() ||
      pieces_by_type_[static_cast<size_t>(Piece::kQueen)].Any()) {
    return false;
  }

  const auto knights =
      pieces_by_type_[static_cast<size_t>(Piece::kKnight)];
  const auto bishops = pieces_by_type_[static_cast<size_t>(Piece::kBishop)];

  if (knights.Any()) {
    return bishops.None() && !knights.MoreThanOne();
  }

  return bishops.None() || (bishops & kDarkSquares).None() ||
         (bishops & kLightSquares).None();
}

const Bitboard &Position::GetPieces(const Player player) const {
  return pieces_by_color_[static_cast<size_t>(player)];
}

template <Piece piece>
Bitboard Position::GetPiecesByType(const Player player) const {
  return pieces_by_color_[static_cast<size_t>(player)] &
         pieces_by_type_[static_cast<size_t>(piece)];
}

template <Piece piece>
Bitboard Position::GetCastlingSquares(CastlingSide side) const {
  static_assert(piece == Piece::kRook || piece == Piece::kKing);
  if constexpr (piece == Piece::kKing) {
    return castling_squares_for_king_[static_cast<size_t>(side_to_move_)]
                                     [static_cast<size_t>(side)];
  }
  if constexpr (piece == Piece::kRook) {
    return castling_squares_for_rook_[static_cast<size_t>(side_to_move_)]
                                     [static_cast<size_t>(side)];
  }
  assert(false);
  return {};
}

Bitboard Position::GetAllPawnAttacks(const Player player) const {
  const auto us = static_cast<size_t>(player);
  const auto pawns = GetPiecesByType<Piece::kPawn>(player);
  return Shift(pawns, kPawnAttackDirections[us][0]) |
         Shift(pawns, kPawnAttackDirections[us][1]);
}

Piece Position::GetPieceAt(const BitIndex index) const {
  assert(IsOk(index));
  return board_[index];
}

BitIndex Position::GetKingSquare(const Player player) const {
  return king_position_[static_cast<size_t>(player)];
}

BitIndex Position::GetCastlingRookSquare(Player player,
                                         CastlingSide side) const {
  return rook_positions_[static_cast<size_t>(player)]
                        [static_cast<size_t>(side)];
}

bool Position::IsUnderAttack(const BitIndex square, const Player us,
                             const Bitboard transparent) const {
  assert(IsOk(square));
  return (Attackers(square, transparent) &
          pieces_by_color_[static_cast<size_t>(Flip(us))])
      .Any();
}

bool Position::IsUnderCheck() const { return IsUnderCheck(side_to_move_); }

bool Position::IsUnderCheck(const Player player) const {
  return IsUnderAttack(GetKingSquare(player), player);
}

Eval Position::EstimatePiece(const Piece piece) const {
  const auto piece_type_idx = static_cast<size_t>(piece);
  return kPieceValues[piece_type_idx](evaluation_data_.non_pawn_material);
}

bool Position::StaticExchangeEvaluation(const Move &move,
                                        const Eval threshold) const {
  const auto us = side_to_move_;
  const auto them = Flip(us);

  const auto from = move.From();
  const auto to = move.To();
  
  Piece captured_piece = Piece::kNone;
  if (move.IsEnPassant()) {
    captured_piece = Piece::kPawn;
  } else if (!move.IsCastling()) {
    captured_piece = GetPieceAt(to);
  }

  Piece next_victim = GetPieceAt(from);
  
  if (move.IsPromotion()) {
    next_victim = move.PromotionPiece();
  }

  Eval balance = EstimatePiece(captured_piece);

  if (move.IsPromotion()) {
    balance += EstimatePiece(move.PromotionPiece()) - EstimatePiece(Piece::kPawn);
  } else if (move.IsEnPassant()) {
    balance = EstimatePiece(Piece::kPawn);
  }

  balance -= threshold;

  if (balance < 0) {
    return false;
  }

  balance -= EstimatePiece(next_victim);

  if (balance >= 0) {
    return true;
  }

  Bitboard occupancy = GetAllPieces() ^ SingleSquare(from) ^ SingleSquare(to);

  [[unlikely]] if (move.IsEnPassant()) {
    occupancy ^= Shift(SingleSquare(to), kPawnMoveDirection[static_cast<size_t>(them)]);
  }

  Bitboard attackers = Attackers(to, ~occupancy) & occupancy;

  auto color = them;

  auto diagonal_xray = pieces_by_type_[static_cast<size_t>(Piece::kBishop)] |
                       pieces_by_type_[static_cast<size_t>(Piece::kQueen)];

  auto straight_xray = pieces_by_type_[static_cast<size_t>(Piece::kRook)] |
                       pieces_by_type_[static_cast<size_t>(Piece::kQueen)];

  for (;;) {
    Bitboard our_attackers = attackers & GetPieces(color);
    if (our_attackers.None()) {
      break;
    }

    Bitboard attacker_bitboard;

    next_victim = Piece::kKing;

    for (size_t attacker_idx = static_cast<size_t>(Piece::kPawn);
         attacker_idx <= static_cast<size_t>(Piece::kQueen); ++attacker_idx) {
      if (attacker_bitboard = our_attackers & pieces_by_type_[attacker_idx];
          attacker_bitboard.Any()) {
        next_victim = static_cast<Piece>(attacker_idx);
        break;
      }
    }

    attacker_bitboard &= -attacker_bitboard;

    occupancy ^= attacker_bitboard;

    if (IsDiagonalAttacker(next_victim)) {
      attackers |= AttackTable<Piece::kBishop>::GetAttackMap(to, occupancy) &
                   diagonal_xray;
    }

    if (IsStraightAttacker(next_victim)) {
      attackers |= AttackTable<Piece::kRook>::GetAttackMap(to, occupancy) &
                   straight_xray;
    }

    attackers &= occupancy;

    color = Flip(color);

    balance = -balance - 1 - EstimatePiece(next_victim);

    if (balance >= 0) {
      if (next_victim == Piece::kKing && (attackers & GetPieces(color)).Any()) {
        color = Flip(color);
      }
      break;
    }
  }

  return color != GetSideToMove();
}

const std::optional<BitIndex> &Position::GetEnCroissantSquare() const {
  return irreversible_data_.en_croissant_square;
}

const std::array<std::bitset<2>, kColors> &Position::GetCastlingRights() const {
  return irreversible_data_.castling_rights;
}

Bitboard Position::Attackers(const BitIndex square,
                             const Bitboard transparent) const {
  assert(IsOk(square));
  const Bitboard occupancy = GetAllPieces() & ~transparent;
  return (GetPawnAttacks(square, Player::kWhite) &
          GetPiecesByType<Piece::kPawn>(Player::kBlack)) |
         (GetPawnAttacks(square, Player::kBlack) &
          GetPiecesByType<Piece::kPawn>(Player::kWhite)) |
         (AttackTable<Piece::kKnight>::GetAttackMap(square, occupancy) &
          pieces_by_type_[static_cast<size_t>(Piece::kKnight)]) |
         (AttackTable<Piece::kBishop>::GetAttackMap(square, occupancy) &
          pieces_by_type_[static_cast<size_t>(Piece::kBishop)]) |
         (AttackTable<Piece::kRook>::GetAttackMap(square, occupancy) &
          pieces_by_type_[static_cast<size_t>(Piece::kRook)]) |
         (AttackTable<Piece::kQueen>::GetAttackMap(square, occupancy) &
          pieces_by_type_[static_cast<size_t>(Piece::kQueen)]) |
         (AttackTable<Piece::kKing>::GetAttackMap(square, occupancy) &
          pieces_by_type_[static_cast<size_t>(Piece::kKing)]);
}

void Position::ComputePins(const Player us) {
  const Player them = Flip(us);

  const auto us_idx = static_cast<size_t>(us);

  irreversible_data_.blockers[us_idx] = kEmptyBoard;
  irreversible_data_.pinners[us_idx] = kEmptyBoard;

  const BitIndex king_square = GetKingSquare(us);

  const Bitboard all_pieces = GetAllPieces();

  const Bitboard diagonal_blockers =
      AttackTable<Piece::kBishop>::GetAttackMap(king_square, all_pieces) &
      all_pieces;
  const Bitboard straight_blockers =
      AttackTable<Piece::kRook>::GetAttackMap(king_square, all_pieces) &
      all_pieces;

  Bitboard diagonal_pinners = AttackTable<Piece::kBishop>::GetAttackMap(
                                  king_square, all_pieces ^ diagonal_blockers) &
                              (GetPiecesByType<Piece::kBishop>(them) |
                               GetPiecesByType<Piece::kQueen>(them)) &
                              ~diagonal_blockers;
  Bitboard straight_pinners = AttackTable<Piece::kRook>::GetAttackMap(
                                  king_square, all_pieces ^ straight_blockers) &
                              (GetPiecesByType<Piece::kRook>(them) |
                               GetPiecesByType<Piece::kQueen>(them)) &
                              ~straight_blockers;

  irreversible_data_.pinners[us_idx] = diagonal_pinners | straight_pinners;

  Bitboard &blockers = irreversible_data_.blockers[us_idx];
  while (diagonal_pinners.Any()) {
    const BitIndex square = diagonal_pinners.PopFirstBit();

    const auto blockers_between =
        bishop_between[square][king_square] & diagonal_blockers;
    assert(blockers_between.Any() && !blockers_between.MoreThanOne());
    blockers |= blockers_between;
  }
  while (straight_pinners.Any()) {
    const BitIndex square = straight_pinners.PopFirstBit();

    const auto blockers_between =
        rook_between[square][king_square] & straight_blockers;
    assert(blockers_between.Any() && !blockers_between.MoreThanOne());
    blockers |= blockers_between;
  }
}

Position::IrreversibleData Position::GetIrreversibleData() const {
  return irreversible_data_;
}

size_t Position::GameHistory::Count(const Hash hash, Depth depth) const {
  size_t result = 0;
  size_t current_result = 0;
  size_t parity_shift = (history.size() ^ last_reset[history.size()] ^ 1) % 2;

  for (int i = history.size() - 1;
       i >= static_cast<int>(last_reset[history.size()] + parity_shift);
       i -= 2) {
    if (history[i] == hash) {
      ++result;
      if (i > static_cast<int>(history.size() - 1 - depth)) ++current_result;
    }
  }

  if (current_result >= 2) {
    return 3;
  }
  return result;
}
}  // namespace SimpleChessEngine
