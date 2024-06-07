#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <random>

#include "Attacks.h"
#include "Bitboard.h"
#include "Evaluation.h"
#include "Hasher.h"
#include "Move.h"
#include "PSQT.h"
#include "Piece.h"
#include "Player.h"
#include "Utility.h"

namespace SimpleChessEngine {
/**
 * \brief Class that represents a chess position.
 *
 * \author nook0110
 * \author alfoos
 */
class Position {
 public:
  struct EvaluationData {
    bool operator==(const EvaluationData&) const = default;

    Eval non_pawn_material{};
    std::array<TaperedEval, kColors> material{};
    std::array<TaperedEval, kColors> psqt{};
  };

  [[nodiscard]] Eval Evaluate() const;

  struct IrreversibleData {
    std::optional<BitIndex> en_croissant_square{};

    std::array<std::bitset<2>, kColors> castling_rights{};

    std::array<Bitboard, kColors> pinners{};
    std::array<Bitboard, kColors> blockers{};

    bool operator==(const IrreversibleData& other) const {
      return std::tie(en_croissant_square, castling_rights) ==
             std::tie(other.en_croissant_square, other.castling_rights);
    }
  };

  /**
   * \brief Places a piece with a color on a chosen square.
   *
   * \param square Square to place piece.
   * \param piece Piece to place.
   * \param color Color of the piece.
   *
   */
  void PlacePiece(const BitIndex square, const Piece piece,
                  const Player color) {
    assert(IsOk(square));
    assert(!board_[square]);
    assert(piece);
    const auto piece_idx = static_cast<size_t>(piece);
    const auto color_idx = static_cast<size_t>(color);
    board_[square] = piece;
    pieces_by_type_[piece_idx].Set(square);
    pieces_by_color_[color_idx].Set(square);
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
  void RemovePiece(const BitIndex square, const Player color) {
    assert(IsOk(square));
    const auto piece = board_[square];
    assert(!!piece);
    const auto piece_idx = static_cast<size_t>(piece);
    const auto color_idx = static_cast<size_t>(color);
    pieces_by_type_[piece_idx].Reset(square);
    pieces_by_color_[color_idx].Reset(square);
    board_[square] = Piece::kNone;
    evaluation_data_.material[color_idx] -= kPieceValues[piece_idx];
    evaluation_data_.psqt[color_idx] -= kPSQT[color_idx][piece_idx][square];
    if (piece != Piece::kPawn)
      evaluation_data_.non_pawn_material -= kPieceValues[piece_idx].eval[0];
    hash_ ^= hasher_.psqt_hash[piece_idx][color_idx][square];
  }

  void MovePiece(const BitIndex from, const BitIndex to, const Player color) {
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

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const Move& move);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const DefaultMove& move);

  void DoMove(const PawnPush& move);

  void DoMove(const DoublePush& move);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const EnCroissant& move);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const Promotion& move);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */

  void DoMove(const Castling& move);

  /**
   * \brief Undoes given move.
   *
   * \param move Move to undo.
   */
  void UndoMove(const Move& move, const IrreversibleData& data);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void UndoMove(const DefaultMove& move);

  void UndoMove(const PawnPush& move);

  void UndoMove(const DoublePush& move);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void UndoMove(const EnCroissant& move);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void UndoMove(const Promotion& move);

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void UndoMove(const Castling& move);

  [[nodiscard]] bool CanCastle(
      const Castling::CastlingSide castling_side) const {
    const auto us = side_to_move_;
    const auto us_idx = static_cast<size_t>(us);
    const auto cs_idx = static_cast<size_t>(castling_side);

    if (!irreversible_data_.castling_rights[us_idx].test(cs_idx)) return false;

    const auto king_position = king_position_[us_idx];
    const auto rook_position = rook_positions_[us_idx][cs_idx];

    const auto obstacles = GetAllPieces() &
                           ~GetBitboardOfSquare(king_position) &
                           ~GetBitboardOfSquare(rook_position);

    auto king_path = castling_squares_for_king_[us_idx][cs_idx];

    if (((king_path | castling_squares_for_rook_[us_idx][cs_idx]) & obstacles)
            .Any())
      return false;

    while (king_path.Any()) {
      if (const auto square = king_path.PopFirstBit();
          IsUnderAttack(square, us))
        return false;
    }

    return true;
  }

  void SetCastlingRights(const std::array<std::bitset<2>, 2>& castling_rights) {
    irreversible_data_.castling_rights = castling_rights;
  }

  void SetKingPositions(const std::array<BitIndex, 2>& king_position) {
    king_position_ = king_position;
  }

  void SetRookPositions(
      const std::array<std::array<BitIndex, 2>, kColors>& rook_positions) {
    rook_positions_ = rook_positions;
  }

  void SetCastlingSquares(
      const std::array<std::array<Bitboard, 2>, kColors>& cs_king,
      const std::array<std::array<Bitboard, 2>, kColors>& cs_rook) {
    castling_squares_for_king_ = cs_king;
    castling_squares_for_rook_ = cs_rook;
  }

  /**
   * \brief Gets hash of the position.
   *
   * \return Hash of the position.
   */
  [[nodiscard]] Hash GetHash() const { return hash_; }

  /**
   * \brief Gets all pieces on the board.
   *
   * \return Bitboard with all pieces on the board.
   */
  [[nodiscard]] Bitboard GetAllPieces() const;

  /**
   * \brief Gets all pieces of given player.
   *
   * \param player Player to get pieces of.
   *
   * \return Bitboard with all pieces of given player.
   */
  [[nodiscard]] const Bitboard& GetPieces(Player player) const;

  template <Piece piece>
  [[nodiscard]] Bitboard GetPiecesByType(Player player) const;

  /**
   * \brief Gets piece on given square.
   *
   * \param index Square to get piece from.
   *
   * \return Piece on given square.
   */
  [[nodiscard]] Piece GetPiece(BitIndex index) const;

  /**
   * \brief Gets side to move.
   *
   * \return Side to move.
   */
  [[nodiscard]] Player GetSideToMove() const { return side_to_move_; }

  /**
   * \brief Sets side to move.
   *
   * \param player Player whose's side to move.
   */
  void SetSideToMove(const Player player) { side_to_move_ = player; }

  [[nodiscard]] Bitboard GetAllPawnAttacks(Player player) const;

  [[nodiscard]] BitIndex GetKingSquare(Player player) const;

  [[nodiscard]] BitIndex GetCastlingRookSquare(
      Player player, Castling::CastlingSide side) const;

  [[nodiscard]] Bitboard Attackers(BitIndex square,
                                   Bitboard transparent = kEmptyBoard) const;

  void ComputePins(Player us);

  [[nodiscard]] bool IsUnderAttack(BitIndex square, Player us,
                                   Bitboard transparent = kEmptyBoard) const;

  /**
   * \brief Checks if current player is under check.
   *
   * \return True if current player is under check, false otherwise.
   */
  [[nodiscard]] bool IsUnderCheck() const;

  /**
   * \brief Checks if given player is under check.
   *
   * \return True if given player is under check, false otherwise.
   */
  [[nodiscard]] bool IsUnderCheck(Player player) const;

  [[nodiscard]] const std::optional<BitIndex>& GetEnCroissantSquare() const;

  [[nodiscard]] const std::array<std::bitset<2>, kColors>& GetCastlingRights()
      const;

  template <Piece piece>
  [[nodiscard]] Bitboard GetCastlingSquares(Castling::CastlingSide side) const;

  [[nodiscard]] IrreversibleData GetIrreversibleData() const;
  /**
   * \brief Default operator==()
   *
   * \param other Other position to compare with.
   *
   * \return True if positions are the same, false otherwise.
   */
  bool operator==(const Position& other) const {
    return std::tie(hash_, side_to_move_, pieces_by_type_, pieces_by_color_,
                    irreversible_data_) ==
           std::tie(other.hash_, other.side_to_move_, other.pieces_by_type_,
                    other.pieces_by_color_, other.irreversible_data_);
  }

 private:
  EvaluationData evaluation_data_;
  IrreversibleData irreversible_data_;

  Player side_to_move_{};  //!< Whose side to move.

  std::array<Bitboard, kPieceTypes>
      pieces_by_type_{};  //!< Bitboard of pieces of certain type
  std::array<Bitboard, kColors>
      pieces_by_color_{};  //!< Bitboard of pieces for each player

  std::array<Piece, kBoardArea> board_{};  //!< Current position of pieces

  std::array<BitIndex, kColors> king_position_{};
  std::array<std::array<BitIndex, 2>, kColors>
      rook_positions_{};  // doesn't need to be updated

  std::array<std::array<Bitboard, 2>, kColors> castling_squares_for_king_{};
  std::array<std::array<Bitboard, 2>, kColors> castling_squares_for_rook_{};

  Hash hash_{};
  Hasher hasher_{std::mt19937_64(80085)};
};

inline Bitboard Position::GetAllPieces() const {
  return pieces_by_color_[static_cast<size_t>(Player::kWhite)] |
         pieces_by_color_[static_cast<size_t>(Player::kBlack)];
}

inline const Bitboard& Position::GetPieces(const Player player) const {
  return pieces_by_color_[static_cast<size_t>(player)];
}

template <Piece piece>
Bitboard Position::GetPiecesByType(const Player player) const {
  return pieces_by_color_[static_cast<size_t>(player)] &
         pieces_by_type_[static_cast<size_t>(piece)];
}

template <Piece piece>
Bitboard Position::GetCastlingSquares(Castling::CastlingSide side) const {
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

inline Bitboard Position::GetAllPawnAttacks(const Player player) const {
  const auto us = static_cast<size_t>(player);
  const auto pawns = GetPiecesByType<Piece::kPawn>(player);
  return Shift(pawns, kPawnAttackDirections[us][0]) |
         Shift(pawns, kPawnAttackDirections[us][1]);
}

inline Piece Position::GetPiece(const BitIndex index) const {
  assert(IsOk(index));
  return board_[index];
}

inline BitIndex Position::GetKingSquare(const Player player) const {
  return king_position_[static_cast<size_t>(player)];
}

inline BitIndex Position::GetCastlingRookSquare(
    Player player, Castling::CastlingSide side) const {
  return rook_positions_[static_cast<size_t>(player)]
                        [static_cast<size_t>(side)];
}

inline bool Position::IsUnderAttack(const BitIndex square, const Player us,
                                    const Bitboard transparent) const {
  assert(IsOk(square));
  return (Attackers(square, transparent) &
          pieces_by_color_[static_cast<size_t>(Flip(us))])
      .Any();
}

inline bool Position::IsUnderCheck() const {
  return IsUnderCheck(side_to_move_);
}

inline bool Position::IsUnderCheck(const Player player) const {
  return IsUnderAttack(GetKingSquare(player), player);
}

inline const std::optional<BitIndex>& Position::GetEnCroissantSquare() const {
  return irreversible_data_.en_croissant_square;
}

inline const std::array<std::bitset<2>, kColors>& Position::GetCastlingRights()
    const {
  return irreversible_data_.castling_rights;
}

inline Bitboard Position::Attackers(const BitIndex square,
                                    const Bitboard transparent) const {
  assert(IsOk(square));
  const Bitboard occupancy = GetAllPieces() & ~transparent;
  return GetPawnAttacks(square, Player::kWhite) &
             GetPiecesByType<Piece::kPawn>(Player::kBlack) |
         GetPawnAttacks(square, Player::kBlack) &
             GetPiecesByType<Piece::kPawn>(Player::kWhite) |
         AttackTable<Piece::kKnight>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kKnight)] |
         AttackTable<Piece::kBishop>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kBishop)] |
         AttackTable<Piece::kRook>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kRook)] |
         AttackTable<Piece::kQueen>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kQueen)] |
         AttackTable<Piece::kKing>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kKing)];
}

inline void Position::ComputePins(const Player us) {
  const Player them = Flip(us);

  const auto us_idx = static_cast<size_t>(us);

  irreversible_data_.blockers[us_idx] = kEmptyBoard;
  irreversible_data_.pinners[us_idx] = kEmptyBoard;

  const BitIndex king_square = GetKingSquare(us);

  const Bitboard all_pieces = GetAllPieces();

  const Bitboard diagonal_blockers =
      AttackTable<Piece::kBishop>::GetAttackMap(king_square, all_pieces) &
      all_pieces;
  const Bitboard horizontal_blockers =
      AttackTable<Piece::kRook>::GetAttackMap(king_square, all_pieces) &
      all_pieces;

  Bitboard diagonal_pinners = AttackTable<Piece::kBishop>::GetAttackMap(
                                  king_square, all_pieces ^ diagonal_blockers) &
                              (GetPiecesByType<Piece::kBishop>(them) |
                               GetPiecesByType<Piece::kQueen>(them)) &
                              ~diagonal_blockers;
  Bitboard horizontal_pinners =
      AttackTable<Piece::kRook>::GetAttackMap(
          king_square, all_pieces ^ horizontal_blockers) &
      (GetPiecesByType<Piece::kRook>(them) |
       GetPiecesByType<Piece::kQueen>(them)) &
      ~horizontal_blockers;

  irreversible_data_.pinners[us_idx] = diagonal_pinners | horizontal_pinners;

  Bitboard& blockers = irreversible_data_.blockers[us_idx];
  while (diagonal_pinners.Any()) {
    const BitIndex square = diagonal_pinners.PopFirstBit();

    const auto blockers_between =
        bishop_between[square][king_square] & diagonal_blockers;
    assert(blockers_between.Any() && !blockers_between.MoreThanOne());
    blockers |= blockers_between;
  }
  while (horizontal_pinners.Any()) {
    const BitIndex square = horizontal_pinners.PopFirstBit();

    const auto blockers_between =
        rook_between[square][king_square] & horizontal_blockers;
    assert(blockers_between.Any() && !blockers_between.MoreThanOne());
    blockers |= blockers_between;
  }
}

inline Position::IrreversibleData Position::GetIrreversibleData() const {
  return irreversible_data_;
}
}  // namespace SimpleChessEngine