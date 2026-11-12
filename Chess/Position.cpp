#include "Position.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "Attacks.h"
#include "BitBoard.h"
#include "MoveGenerator.h"
#include "Nnue/Stockfish/position.h"
#include "PSQT.h"
#include "Piece.h"

namespace SimpleChessEngine {
static_assert(std::to_underlying(Player::kWhite) ==
              std::to_underlying(Stockfish::WHITE));
static_assert(std::to_underlying(Player::kBlack) ==
              std::to_underlying(Stockfish::BLACK));
static_assert([] {
  constexpr std::array mapping{
      std::pair{Piece::kNone, Stockfish::NO_PIECE_TYPE},
      std::pair{Piece::kPawn, Stockfish::PAWN},
      std::pair{Piece::kKnight, Stockfish::KNIGHT},
      std::pair{Piece::kBishop, Stockfish::BISHOP},
      std::pair{Piece::kRook, Stockfish::ROOK},
      std::pair{Piece::kQueen, Stockfish::QUEEN},
      std::pair{Piece::kKing, Stockfish::KING}};
  for (const auto& [piece, stockfish_piece] : mapping) {
    if (std::to_underlying(piece) != std::to_underlying(stockfish_piece))
      return false;
  }
  return true;
}());

template Bitboard Position::GetPiecesByType<Piece::kPawn>(Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kKnight>(
    Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kBishop>(
    Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kRook>(Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kQueen>(Player player) const;
template Bitboard Position::GetPiecesByType<Piece::kKing>(Player player) const;


Eval Position::SimpleMaterialEval() const {
  return evaluation_data_.material[side_to_move_].eval[0] -
         evaluation_data_.material[Flip(side_to_move_)].eval[0];
}

/**
 * \brief Places a piece with a color on a chosen square.
 *
 * \param square Square to place piece.
 * \param piece Piece to place.
 * \param color Color of the piece.
 *
 */
void Position::PlacePiece(const BitIndex square, const Piece piece,
                          const Player color,
                          Stockfish::DirtyThreats *threats) {
  assert(IsOk(square));
  assert(!board_[square]);
  assert(piece);
  board_[square] = piece;
  pieces_by_type_[piece].Set(square);
  pieces_by_color_[color].Set(square);
  ++evaluation_data_.piece_counts[color][piece];
  evaluation_data_.material[color] += kPieceValues[piece];
  evaluation_data_.psqt[color] += kPSQT[color][piece][square];
  if (piece != Piece::kPawn)
    evaluation_data_.non_pawn_material += kPieceValues[piece].eval[0];
  hash_ ^= kZobristKeys.psqt_hash[piece][color][square];
  if (threats)
    Stockfish::Position{*this}.update_piece_threats(
        Stockfish::make_piece(static_cast<Stockfish::Color>(color),
                              static_cast<Stockfish::PieceType>(piece)),
        true, static_cast<Stockfish::Square>(square), threats);
}

/**
 * \brief Removes a piece from a chosen square.
 *
 * \param square Square to remove piece from.
 * \param color Color of the piece.
 *
 */

void Position::RemovePiece(const BitIndex square, const Player color,
                           Stockfish::DirtyThreats *threats) {
  assert(IsOk(square));
  const auto piece = board_[square];
  assert(!!piece);
  if (threats)
    Stockfish::Position{*this}.update_piece_threats(
        Stockfish::make_piece(static_cast<Stockfish::Color>(color),
                              static_cast<Stockfish::PieceType>(piece)),
        false, static_cast<Stockfish::Square>(square), threats);
  pieces_by_type_[piece].Reset(square);
  pieces_by_color_[color].Reset(square);
  --evaluation_data_.piece_counts[color][piece];
  board_[square] = Piece::kNone;
  evaluation_data_.material[color] -= kPieceValues[piece];
  evaluation_data_.psqt[color] -= kPSQT[color][piece][square];
  if (piece != Piece::kPawn)
    evaluation_data_.non_pawn_material -= kPieceValues[piece].eval[0];
  hash_ ^= kZobristKeys.psqt_hash[piece][color][square];
}

void Position::MovePiece(const BitIndex from, const BitIndex to,
                         const Player color, Stockfish::DirtyThreats *threats) {
  assert(IsOk(from));
  assert(IsOk(to));
  const auto piece = board_[from];
  assert(!!piece);
  assert(!board_[to]);
  const auto sf_piece =
      Stockfish::make_piece(static_cast<Stockfish::Color>(color),
                            static_cast<Stockfish::PieceType>(piece));
  const auto from_to =
      Stockfish::square_bb(static_cast<Stockfish::Square>(from)) |
      Stockfish::square_bb(static_cast<Stockfish::Square>(to));
  if (threats)
    Stockfish::Position{*this}.update_piece_threats(
        sf_piece, false, static_cast<Stockfish::Square>(from), threats,
        from_to);
  pieces_by_type_[piece].Reset(from);
  pieces_by_color_[color].Reset(from);
  pieces_by_type_[piece].Set(to);
  pieces_by_color_[color].Set(to);
  board_[from] = Piece::kNone;
  board_[to] = piece;
  evaluation_data_.psqt[color] -= kPSQT[color][piece][from];
  evaluation_data_.psqt[color] += kPSQT[color][piece][to];
  hash_ ^= kZobristKeys.psqt_hash[piece][color][from];
  hash_ ^= kZobristKeys.psqt_hash[piece][color][to];
  if (threats)
    Stockfish::Position{*this}.update_piece_threats(
        sf_piece, true, static_cast<Stockfish::Square>(to), threats, from_to);
}

bool Position::VerifyPinCache() const {
  const auto cached = irreversible_data_;
  bool valid = true;
  for (const auto player : {Player::kWhite, Player::kBlack}) {
    const auto side = player;
    if (!cached.pins_valid[side]) continue;
    irreversible_data_.pins_valid[side] = false;
    ComputePins(player);
    valid &= irreversible_data_.blockers[side] == cached.blockers[side] &&
             irreversible_data_.pinners[side] == cached.pinners[side];
    irreversible_data_.blockers[side] = cached.blockers[side];
    irreversible_data_.pinners[side] = cached.pinners[side];
    irreversible_data_.pins_valid[side] = cached.pins_valid[side];
  }
  return valid;
}

bool Position::VerifyIncrementalState() const {
  static thread_local std::uint64_t checks = 0;
  if ((checks++ & 4095u) != 0) return true;

  ByPiece<Bitboard> pieces_by_type{};
  ByPlayer<Bitboard> pieces_by_color{};
  ByPlayer<BitIndex> king_position{};
  ByPlayer<int> king_count{};
  EvaluationData evaluation;
  Hash hash = side_to_move_ == Player::kBlack ? kZobristKeys.stm_hash : 0;
  for (std::size_t index = 0; index < kBoardArea; ++index) {
    const auto square = static_cast<BitIndex>(index);
    const auto piece = board_[square];
    const bool white = pieces_by_color_[Player::kWhite].Test(square);
    const bool black = pieces_by_color_[Player::kBlack].Test(square);
    if (piece == Piece::kNone) {
      if (white || black) return false;
      continue;
    }
    if (white == black) return false;
    const auto color = white ? Player::kWhite : Player::kBlack;
    pieces_by_type[piece].Set(square);
    pieces_by_color[color].Set(square);
    ++evaluation.piece_counts[color][piece];
    evaluation.material[color] += kPieceValues[piece];
    evaluation.psqt[color] += kPSQT[color][piece][square];
    if (piece != Piece::kPawn) {
      evaluation.non_pawn_material += kPieceValues[piece].eval[0];
    }
    if (piece == Piece::kKing) {
      king_position[color] = square;
      ++king_count[color];
    }
    hash ^= kZobristKeys.psqt_hash[piece][color][square];
  }
  for (const auto color : kPlayers) {
    hash ^= kZobristKeys.cr_hash[color][0];
    hash ^= kZobristKeys
                .cr_hash[color]
                        [irreversible_data_.castling_rights[color].to_ulong()];
  }
  if (irreversible_data_.en_croissant_square) {
    hash ^= kZobristKeys.en_croissant_hash
                [GetCoordinates(*irreversible_data_.en_croissant_square).first];
  }
  if (pieces_by_type != pieces_by_type_ ||
      pieces_by_color != pieces_by_color_ ||
      (pieces_by_color[Player::kWhite] | pieces_by_color[Player::kBlack]) != GetAllPieces() ||
      king_count[Player::kWhite] != 1 || king_count[Player::kBlack] != 1 ||
      king_position != king_position_ || evaluation != evaluation_data_ ||
      hash != hash_) {
    return false;
  }
  if (!VerifyPinCache()) return false;
  if (!Nnue::IsLoaded()) return true;
  if (!Nnue::VerifyAccumulators(*this, nnue_)) return false;
  const auto incremental = EvaluateNetwork();
  const auto scratch = Nnue::EvaluateFromScratch(*this);
  return incremental.psqt == scratch.psqt &&
         incremental.positional == scratch.positional;
}

void Position::DoMove(const LegalMove &move) {
  irreversible_data_.pins_valid = {};
  if (const auto &ep_square = irreversible_data_.en_croissant_square;
      ep_square.has_value()) {
    hash_ ^=
        kZobristKeys.en_croissant_hash[GetCoordinates(ep_square.value()).first];
    irreversible_data_.en_croissant_square.reset();
  }
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= kZobristKeys
                 .cr_hash[color]
                         [irreversible_data_.castling_rights[color].to_ulong()];
  }

  const auto from = move.From();
  const auto to = move.To();
  const auto us = side_to_move_;
  const auto them = Flip(us);
  const auto moving_piece = board_[from];
  
  // Store captured piece for undo
  irreversible_data_.captured_piece = board_[to];
  auto &dirties = nnue_.Push();
  auto &dirty = dirties.dirtyPiece;
  auto *threats = Nnue::IsLoaded() ? &dirties.dirtyThreats : nullptr;
  const auto sf_piece = [](Piece piece, Player color) {
    return Stockfish::make_piece(static_cast<Stockfish::Color>(color),
                                 static_cast<Stockfish::PieceType>(piece));
  };
  dirty = {sf_piece(moving_piece, us),
           static_cast<Stockfish::Square>(from),
           static_cast<Stockfish::Square>(to),
           Stockfish::SQ_NONE,
           Stockfish::SQ_NONE,
           Stockfish::NO_PIECE,
           Stockfish::NO_PIECE};
  const auto swap_piece = [&](BitIndex square, Piece replacement) {
    const auto old = sf_piece(board_[square], them);
    RemovePiece(square, them);
    if (threats)
      Stockfish::Position{*this}.update_piece_threats<false>(
          old, false, static_cast<Stockfish::Square>(square), threats);
    PlacePiece(square, replacement, us);
    if (threats)
      Stockfish::Position{*this}.update_piece_threats<false>(
          sf_piece(replacement, us), true,
          static_cast<Stockfish::Square>(square), threats);
  };
  for (const auto color : {Player::kWhite, Player::kBlack})
    dirties.dirtyPawnPairs.before[std::to_underlying(color)] =
        static_cast<std::uint64_t>(GetPiecesOf(color, Piece::kPawn));

  if (move.IsEnPassant()) {
    const auto capture_square = Shift(to, kPawnMoveDirection[them]);
    irreversible_data_.captured_piece = Piece::kPawn;  // En passant always captures a pawn
    dirty.remove_pc = sf_piece(Piece::kPawn, them);
    dirty.remove_sq = static_cast<Stockfish::Square>(capture_square);
    RemovePiece(capture_square, them, threats);
    MovePiece(from, to, us, threats);
  } else if (move.IsPromotion()) {
    const auto promoted_to = move.PromotionPiece();
    const auto captured_piece = board_[to];
    dirty.to = Stockfish::SQ_NONE;
    dirty.add_pc = sf_piece(promoted_to, us);
    dirty.add_sq = static_cast<Stockfish::Square>(to);
    if (!!captured_piece) {
      dirty.remove_pc = sf_piece(captured_piece, them);
      dirty.remove_sq = static_cast<Stockfish::Square>(to);
    }
    RemovePiece(from, us, threats);
    if (!!captured_piece)
      swap_piece(to, promoted_to);
    else
      PlacePiece(to, promoted_to, us, threats);

    for (const auto castling_side : {CastlingSide::k00, CastlingSide::k000}) {
      const auto their_rook = rook_positions_[them][castling_side];
      if (to == their_rook) {
        irreversible_data_.castling_rights[them] &=
            ~static_cast<char>(kCastlingRightsForSide[castling_side]);
      }
    }
  } else if (move.IsCastling()) {
    irreversible_data_.captured_piece = Piece::kNone;  // Castling never captures
    // Determine castling side and rook position from move
    const auto king_to = to;
    const auto king_from = from;
    
    // Determine which side based on king destination
    CastlingSide side;
    BitIndex rook_from;
    
    if (king_to == kKingCastlingDestination[us][CastlingSide::k00]) {
      side = CastlingSide::k00;
      rook_from = rook_positions_[us][CastlingSide::k00];
    } else {
      side = CastlingSide::k000;
      rook_from = rook_positions_[us][CastlingSide::k000];
    }
    
    dirty.remove_pc = dirty.add_pc = sf_piece(Piece::kRook, us);
    dirty.remove_sq = static_cast<Stockfish::Square>(rook_from);
    dirty.add_sq = static_cast<Stockfish::Square>(
        kRookCastlingDestination[us][side]);
    RemovePiece(king_from, us, threats);
    RemovePiece(rook_from, us, threats);
    PlacePiece(kKingCastlingDestination[us][side], Piece::kKing, us,
               threats);
    PlacePiece(kRookCastlingDestination[us][side], Piece::kRook, us,
               threats);

    king_position_[us] = kKingCastlingDestination[us][side];
    irreversible_data_.castling_rights[us] = 0;
  } else {
    // Normal move (including pawn pushes and double pushes)
    const auto piece_to_move = board_[from];
    const auto captured_piece = board_[to];
    
    // Check if it's a double pawn push
    if (piece_to_move == Piece::kPawn && std::abs(from - to) == 16) {
      const BitIndex passed_square = std::midpoint(from, to);
      if ((GetAllPawnAttacks(them) & SingleSquare(passed_square)).Any()) {
        hash_ ^= kZobristKeys.en_croissant_hash[GetCoordinates(from).first];
        irreversible_data_.en_croissant_square = passed_square;
      }
    }
    
    if (!!captured_piece) {
      dirty.remove_pc = sf_piece(captured_piece, them);
      dirty.remove_sq = static_cast<Stockfish::Square>(to);
      RemovePiece(from, us, threats);
      swap_piece(to, piece_to_move);
    } else {
      MovePiece(from, to, us, threats);
    }

    if (piece_to_move == Piece::kKing) {
      king_position_[us] = to;
      irreversible_data_.castling_rights[us] = 0;
    }
    
    for (auto castling_side : {CastlingSide::k00, CastlingSide::k000}) {
      const auto our_rook = rook_positions_[us][castling_side];
      const auto their_rook = rook_positions_[them][castling_side];
      if (from == our_rook) {
        irreversible_data_.castling_rights[us] &=
            ~static_cast<std::int8_t>(kCastlingRightsForSide[castling_side]);
      }
      if (to == their_rook) {
        irreversible_data_.castling_rights[them] &=
            ~static_cast<std::int8_t>(kCastlingRightsForSide[castling_side]);
      }
    }
  }

  for (const auto color : {Player::kWhite, Player::kBlack})
    dirties.dirtyPawnPairs.after[std::to_underlying(color)] =
        static_cast<std::uint64_t>(GetPiecesOf(color, Piece::kPawn));

  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= kZobristKeys
                 .cr_hash[color]
                         [irreversible_data_.castling_rights[color].to_ulong()];
  }
  side_to_move_ = Flip(side_to_move_);
  hash_ ^= kZobristKeys.stm_hash;

  // 50-move rule: reset counter if it's a pawn move or a capture
  const bool resets_fifty_move =
      (moving_piece == Piece::kPawn) ||  // Moving piece is a pawn
      (irreversible_data_.captured_piece != Piece::kNone);  // Any capture
  history_stack_.Push(hash_, resets_fifty_move);
  assert(VerifyIncrementalState());
}

void SimpleChessEngine::Position::DoMove(NullMove) {
  if (const auto &ep_square = irreversible_data_.en_croissant_square;
      ep_square.has_value()) {
    hash_ ^=
        kZobristKeys.en_croissant_hash[GetCoordinates(ep_square.value()).first];
    irreversible_data_.en_croissant_square.reset();
  }
  side_to_move_ = Flip(side_to_move_);
  hash_ ^= kZobristKeys.stm_hash;
  auto &dirties = nnue_.Push();
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    const auto pawns =
        static_cast<std::uint64_t>(GetPiecesOf(color, Piece::kPawn));
    dirties.dirtyPawnPairs.before[std::to_underlying(color)] = pawns;
    dirties.dirtyPawnPairs.after[std::to_underlying(color)] = pawns;
  }

  history_stack_.Push(hash_, false);
  assert(VerifyIncrementalState());
}


void Position::UndoMove(const LegalMove &move, const IrreversibleData &data) {
  const auto &ep_square = irreversible_data_.en_croissant_square;
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= kZobristKeys
                 .cr_hash[color]
                         [irreversible_data_.castling_rights[color].to_ulong()];
  }
  if (ep_square.has_value()) {
    hash_ ^=
        kZobristKeys.en_croissant_hash[GetCoordinates(ep_square.value()).first];
  }
  
  const auto from = move.From();
  const auto to = move.To();
  const auto them = side_to_move_;  // Current side (after the move was made)
  const auto us = Flip(them);       // Side that made the move
  
  const auto captured_piece = irreversible_data_.captured_piece;
  
  irreversible_data_ = data;
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    hash_ ^= kZobristKeys
                 .cr_hash[color]
                         [irreversible_data_.castling_rights[color].to_ulong()];
  }
  if (irreversible_data_.en_croissant_square.has_value()) {
    hash_ ^= kZobristKeys.en_croissant_hash
                 [GetCoordinates(irreversible_data_.en_croissant_square.value())
                      .first];
  }
  hash_ ^= kZobristKeys.stm_hash;
  side_to_move_ = Flip(side_to_move_);

  if (move.IsEnPassant()) {
    const auto capture_square = Shift(to, kPawnMoveDirection[them]);
    MovePiece(to, from, us);
    PlacePiece(capture_square, Piece::kPawn, them);
  } else if (move.IsPromotion()) {
    RemovePiece(to, us);
    if (!!captured_piece) PlacePiece(to, captured_piece, them);
    PlacePiece(from, Piece::kPawn, us);
  } else if (move.IsCastling()) {
    
    CastlingSide side;
    if (to == kKingCastlingDestination[us][CastlingSide::k00]) {
      side = CastlingSide::k00;
    } else {
      side = CastlingSide::k000;
    }
    
    const auto rook_from = rook_positions_[us][side];
    
    RemovePiece(kKingCastlingDestination[us][side], us);
    RemovePiece(kRookCastlingDestination[us][side], us);
    
    PlacePiece(from, Piece::kKing, us);
    PlacePiece(rook_from, Piece::kRook, us);
    
    king_position_[us] = from;
  } else {
    // Normal move
    const auto piece_to_move = board_[to];
    
    MovePiece(to, from, us);
    if (!!captured_piece) PlacePiece(to, captured_piece, them);
    
    if (piece_to_move == Piece::kKing) {
      king_position_[us] = from;
    }
  }

  nnue_.Pop();
  history_stack_.Pop();
  assert(VerifyIncrementalState());
}

void SimpleChessEngine::Position::UndoMove(NullMove,
                                           const IrreversibleData &data) {
  irreversible_data_ = data;
  const auto &ep_square = irreversible_data_.en_croissant_square;
  if (ep_square.has_value()) {
    hash_ ^=
        kZobristKeys.en_croissant_hash[GetCoordinates(ep_square.value()).first];
  }
  hash_ ^= kZobristKeys.stm_hash;
  side_to_move_ = Flip(side_to_move_);
  nnue_.Pop();

  history_stack_.Pop();
  assert(VerifyIncrementalState());
}

bool Position::CanPseudoCastle(CastlingSide side) const {
  if (!irreversible_data_.castling_rights[side_to_move_].test(
          std::to_underlying(side))) {
    return false;
  }
  const auto obstacles = GetAllPieces() & ~SingleSquare(king_position_[side_to_move_]) &
                         ~SingleSquare(rook_positions_[side_to_move_][side]);
  return ((castling_squares_for_king_[side_to_move_][side] |
           castling_squares_for_rook_[side_to_move_][side]) &
          obstacles)
      .None();
}

bool Position::IsPseudoLegal(const Move &move) {
  if (!move.IsValid()) return false;
  const auto from = move.From();
  const auto to = move.To();
  if (!GetPieces(side_to_move_).Test(from)) return false;
  if (!move.IsNormal()) {
    return std::ranges::contains(
        MoveGenerator{}.GeneratePseudoLegalMoves<MoveGenerator::Type::kAll>(
            *this),
        move);
  }
  if ((move.Raw() & 0x3000) || from == to || GetPieces(side_to_move_).Test(to))
    return false;
  const auto piece = GetPieceAt(from);
  const auto occupancy = GetAllPieces();
  Bitboard attacks;
  switch (piece) {
    case Piece::kPawn: {
      if ((kRankBB[0] | kRankBB[7]).Test(to)) return false;
      const int step = side_to_move_ == Player::kWhite ? 8 : -8;
      const auto start_rank =
          side_to_move_ == Player::kWhite ? kRankBB[1] : kRankBB[6];
      if (!(GetPawnAttacks(from, side_to_move_) &
            GetPieces(Flip(side_to_move_)))
               .Test(to) &&
          !(to == from + step && !occupancy.Test(to)) &&
          !(to == from + 2 * step && start_rank.Test(from) &&
            !occupancy.Test(to) && !occupancy.Test(from + step)))
        return false;
      break;
    }
    case Piece::kKnight:
      attacks = AttackTable<Piece::kKnight>::GetAttackMap(from, occupancy);
      break;
    case Piece::kBishop:
      attacks = AttackTable<Piece::kBishop>::GetAttackMap(from, occupancy);
      break;
    case Piece::kRook:
      attacks = AttackTable<Piece::kRook>::GetAttackMap(from, occupancy);
      break;
    case Piece::kQueen:
      attacks = AttackTable<Piece::kQueen>::GetAttackMap(from, occupancy);
      break;
    case Piece::kKing:
      attacks = AttackTable<Piece::kKing>::GetAttackMap(from, occupancy);
      break;
    default:
      return false;
  }
  if (piece != Piece::kPawn && !attacks.Test(to)) return false;
  const auto king = GetKingSquare(side_to_move_);
  const auto checkers = Attackers(king) & GetPieces(Flip(side_to_move_));
  if (checkers.Any() && piece != Piece::kKing &&
      (checkers.MoreThanOne() ||
       !(Between(king, checkers.GetFirstBit()) | checkers).Test(to)))
    return false;
  return true;
}

bool Position::IsLegal(const Move &move) const {
  return IsLegal(
      move, irreversible_data_.blockers[side_to_move_]);
}

bool Position::IsLegal(const Move &move, Bitboard blockers) const {
  const auto us = side_to_move_;
  const auto them = Flip(us);
  const auto from = move.From();
  const auto to = move.To();
  if (move.IsCastling()) {
    return !IsUnderCheck() &&
           CanCastle(to == kKingCastlingDestination[us][CastlingSide::k00]
                         ? CastlingSide::k00
                         : CastlingSide::k000);
  }
  if (move.IsEnPassant()) {
    const auto captured =
        static_cast<BitIndex>(to + (us == Player::kWhite ? -8 : 8));
    const auto occupancy =
        (GetAllPieces() ^ SingleSquare(from) ^ SingleSquare(captured)) |
        SingleSquare(to);
    const auto king = GetKingSquare(us);
    return !(AttackTable<Piece::kRook>::GetAttackMap(king, occupancy) &
             (GetPiecesByType<Piece::kRook>(them) |
              GetPiecesByType<Piece::kQueen>(them)))
                .Any() &&
           !(AttackTable<Piece::kBishop>::GetAttackMap(king, occupancy) &
             (GetPiecesByType<Piece::kBishop>(them) |
              GetPiecesByType<Piece::kQueen>(them)))
                .Any();
  }
  if (GetPieceAt(from) == Piece::kKing) {
    return !IsUnderAttack(to, us, SingleSquare(from));
  }
  return !blockers.Test(from) || Ray(GetKingSquare(us), from).Test(to);
}

[[nodiscard]] bool Position::CanCastle(
    const CastlingSide castling_side) const {
  const auto us = side_to_move_;
  if (!irreversible_data_.castling_rights[us].test(
          std::to_underlying(castling_side))) {
    return false;
  }

  const auto king_position = king_position_[us];
  const auto rook_position = rook_positions_[us][castling_side];

  const auto obstacles = GetAllPieces() & ~SingleSquare(king_position) &
                         ~SingleSquare(rook_position);

  const auto king_path = castling_squares_for_king_[us][castling_side];

  if (((king_path | castling_squares_for_rook_[us][castling_side]) & obstacles)
          .Any())
    return false;

  for (const auto square : king_path) {
    if (IsUnderAttack(square, us)) return false;
  }

  return true;
}

void Position::SetCastlingRights(
    const ByPlayer<std::bitset<2>> &castling_rights) {
  for (const auto color : kPlayers) {
    hash_ ^= kZobristKeys
                 .cr_hash[color]
                         [irreversible_data_.castling_rights[color].to_ulong()];
    hash_ ^= kZobristKeys.cr_hash[color][castling_rights[color].to_ulong()];
  }
  irreversible_data_.castling_rights = castling_rights;
}

void Position::SetEnCroissantSquare(std::optional<BitIndex> square) {
  if (square.has_value() &&
      (GetAllPawnAttacks(side_to_move_) & SingleSquare(*square)).None()) {
    square.reset();
  }
  if (irreversible_data_.en_croissant_square.has_value()) {
    hash_ ^=
        kZobristKeys.en_croissant_hash
            [GetCoordinates(*irreversible_data_.en_croissant_square).first];
  }
  irreversible_data_.en_croissant_square = square;
  if (square.has_value()) {
    hash_ ^= kZobristKeys.en_croissant_hash[GetCoordinates(*square).first];
  }
}

void Position::SetKingPositions(const ByPlayer<BitIndex> &king_position) {
  king_position_ = king_position;
}

void Position::SetRookPositions(
    const ByPlayer<ByCastlingSide<BitIndex>> &rook_positions) {
  rook_positions_ = rook_positions;
}

void Position::SetCastlingSquares(
    const ByPlayer<ByCastlingSide<Bitboard>> &cs_king,
    const ByPlayer<ByCastlingSide<Bitboard>> &cs_rook) {
  castling_squares_for_king_ = cs_king;
  castling_squares_for_rook_ = cs_rook;
}

Bitboard Position::GetAllPieces() const {
  return pieces_by_color_[Player::kWhite] |
         pieces_by_color_[Player::kBlack];
}

bool Position::IsInsufficientMaterial() const {
  constexpr Bitboard kDarkSquares{0xAA55AA55AA55AA55ULL};
  constexpr Bitboard kLightSquares{0x55AA55AA55AA55AAULL};

  if (pieces_by_type_[Piece::kPawn].Any() ||
      pieces_by_type_[Piece::kRook].Any() ||
      pieces_by_type_[Piece::kQueen].Any()) {
    return false;
  }

  const auto knights =
      pieces_by_type_[Piece::kKnight];
  const auto bishops = pieces_by_type_[Piece::kBishop];

  if (knights.Any()) {
    return bishops.None() && !knights.MoreThanOne();
  }

  return bishops.None() || (bishops & kDarkSquares).None() ||
         (bishops & kLightSquares).None();
}

const Bitboard &Position::GetPieces(const Player player) const {
  return pieces_by_color_[player];
}

template <Piece piece>
Bitboard Position::GetPiecesByType(const Player player) const {
  return pieces_by_color_[player] &
         pieces_by_type_[piece];
}


Bitboard Position::GetAllPawnAttacks(const Player player) const {
  const auto pawns = GetPiecesByType<Piece::kPawn>(player);
  return Shift(pawns, kPawnAttackDirections[player][0]) |
         Shift(pawns, kPawnAttackDirections[player][1]);
}

Piece Position::GetPieceAt(const BitIndex index) const {
  assert(IsOk(index));
  return board_[index];
}

BitIndex Position::GetKingSquare(const Player player) const {
  return king_position_[player];
}


bool Position::IsUnderAttack(const BitIndex square, const Player us,
                             const Bitboard transparent) const {
  assert(IsOk(square));
  return (Attackers(square, transparent) &
          pieces_by_color_[Flip(us)])
      .Any();
}

bool Position::IsUnderCheck() const { return IsUnderCheck(side_to_move_); }

bool Position::IsUnderCheck(const Player player) const {
  return IsUnderAttack(GetKingSquare(player), player);
}

bool Position::HasLegalKings() const {
  return GetPiecesByType<Piece::kKing>(Player::kWhite).Count() == 1 &&
         GetPiecesByType<Piece::kKing>(Player::kBlack).Count() == 1 &&
         !IsUnderCheck(Flip(side_to_move_));
}

Eval Position::EstimatePiece(const Piece piece) const {
  return kPieceValues[piece](evaluation_data_.non_pawn_material);
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
    occupancy ^= Shift(SingleSquare(to), kPawnMoveDirection[them]);
  }

  Bitboard attackers = Attackers(to, ~occupancy) & occupancy;

  auto color = them;

  auto diagonal_xray = pieces_by_type_[Piece::kBishop] |
                       pieces_by_type_[Piece::kQueen];

  auto straight_xray = pieces_by_type_[Piece::kRook] |
                       pieces_by_type_[Piece::kQueen];

  for (;;) {
    Bitboard our_attackers = attackers & GetPieces(color);
    if (our_attackers.None()) {
      break;
    }

    ComputePins(color);
    if ((irreversible_data_.pinners[color] & occupancy).Any()) {
      our_attackers &= ~irreversible_data_.blockers[color];
      if (our_attackers.None()) {
        break;
      }
    }

    Bitboard attacker_bitboard;

    next_victim = Piece::kKing;

    for (const auto attacker : kPiecesByValue) {
      if (attacker_bitboard = our_attackers & pieces_by_type_[attacker];
          attacker_bitboard.Any()) {
        next_victim = attacker;
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

const ByPlayer<std::bitset<2>> &Position::GetCastlingRights() const {
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
          pieces_by_type_[Piece::kKnight]) |
         (AttackTable<Piece::kBishop>::GetAttackMap(square, occupancy) &
          pieces_by_type_[Piece::kBishop]) |
         (AttackTable<Piece::kRook>::GetAttackMap(square, occupancy) &
          pieces_by_type_[Piece::kRook]) |
         (AttackTable<Piece::kQueen>::GetAttackMap(square, occupancy) &
          pieces_by_type_[Piece::kQueen]) |
         (AttackTable<Piece::kKing>::GetAttackMap(square, occupancy) &
          pieces_by_type_[Piece::kKing]);
}

void Position::ComputePins(const Player us) const {
  const Player them = Flip(us);

  if (irreversible_data_.pins_valid[us]) {
    return;
  }

  irreversible_data_.blockers[us] = kEmptyBoard;
  irreversible_data_.pinners[us] = kEmptyBoard;

  const BitIndex king_square = GetKingSquare(us);

  const Bitboard snipers =
      (AttackTable<Piece::kBishop>::GetAttackMap(king_square, kEmptyBoard) &
       (GetPiecesByType<Piece::kBishop>(them) |
        GetPiecesByType<Piece::kQueen>(them))) |
      (AttackTable<Piece::kRook>::GetAttackMap(king_square, kEmptyBoard) &
       (GetPiecesByType<Piece::kRook>(them) |
        GetPiecesByType<Piece::kQueen>(them)));
  const Bitboard occupancy = GetAllPieces() ^ snipers;

  for (const auto square : snipers) {
    const Bitboard blockers_between = Between(king_square, square) & occupancy;
    if (blockers_between.Any() && !blockers_between.MoreThanOne()) {
      irreversible_data_.blockers[us] |= blockers_between;
      if ((blockers_between & GetPieces(us)).Any()) {
        irreversible_data_.pinners[us] |= SingleSquare(square);
      }
    }
  }
  irreversible_data_.pins_valid[us] = true;
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
