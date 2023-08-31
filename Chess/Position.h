#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <random>

#include "Attacks.h"
#include "Bitboard.h"
#include "Hasher.h"
#include "Move.h"
#include "Piece.h"
#include "Player.h"
#include "Utility.h"

namespace SimpleChessEngine
{
/**
 * \brief Class that represents a chess position.
 *
 * \author nook0110
 * \author alfoos
 */
class Position
{
 public:
  struct EvaluationData
  {};

  struct IrreversibleData
  {
    std::optional<BitIndex> en_croissant_square{};

    std::array<std::bitset<2>, kColors> castling_rights{};

    bool operator==(const IrreversibleData&) const = default;
  };

  /**
   * \brief Places a piece with a color on a chosen square.
   *
   * \param square Square to place piece.
   * \param piece Piece to place.
   * \param color Color of the piece.
   *
   */
  void PlacePiece(const BitIndex square, const Piece piece, const Player color)
  {
    assert(!board_[square]);
    assert(piece);
    board_[square] = piece;
    pieces_by_color_[static_cast<size_t>(color)].Set(square);
    pieces_by_type_[static_cast<size_t>(piece)].Set(square);
    hash_ ^= hasher_.psqt_hash[static_cast<size_t>(piece)][square];
  }

  /**
   * \brief Removes a piece from a chosen square.
   *
   * \param square Square to remove piece from.
   * \param color Color of the piece.
   *
   */
  void RemovePiece(const BitIndex square, const Player color)
  {
    auto piece = board_[square];
    pieces_by_type_[static_cast<size_t>(piece)].Reset(square);
    pieces_by_color_[static_cast<size_t>(color)].Reset(square);
    board_[square] = Piece::kNone;
    hash_ ^= hasher_.psqt_hash[static_cast<size_t>(piece)][square];
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

  void SetKingPositions(const std::array<BitIndex, 2>& king_position)
  {
    king_position_ = king_position;
  }

  void SetRookPositions(const std::array<std::array<BitIndex, 2>, kColors>& rook_positions)
  {
    rook_positions_ = rook_positions;
  }

  void SetCastlingSquares(const std::array<std::array<Bitboard, 2>, kColors>& cs_king,
    const std::array<std::array<Bitboard, 2>, kColors>& cs_rook)
  {
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

  [[nodiscard]] Bitboard Attackers(BitIndex square) const;

  [[nodiscard]] bool IsUnderAttack(BitIndex square, Player us) const;

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
  bool operator==(const Position& other) const = default;

 private:
  IrreversibleData irreversible_data_;

  Player side_to_move_{};  //!< Whose side to move.

  std::array<Bitboard, kPieceTypes>
      pieces_by_type_{};  //!< Bitboard of pieces of certain type
  std::array<Bitboard, kColors>
      pieces_by_color_{};  //!< Bitboard of pieces for each player

  std::array<Piece, kBoardArea> board_{};  //!< Current position of pieces

  std::array<BitIndex, kColors> king_position_{};
  std::array<std::array<BitIndex, 2>, kColors> rook_positions_{}; // doesn't need to be updated

  std::array<std::array<Bitboard, 2>, kColors> castling_squares_for_king_{};
  std::array<std::array<Bitboard, 2>, kColors> castling_squares_for_rook_{};

  Hash hash_{};
  Hasher hasher_{std::mt19937_64(80085)};
};

inline Bitboard Position::GetAllPieces() const
{
  return pieces_by_color_[static_cast<size_t>(Player::kWhite)] |
         pieces_by_color_[static_cast<size_t>(Player::kBlack)];
}

inline const Bitboard& Position::GetPieces(const Player player) const
{
  return pieces_by_color_[static_cast<size_t>(player)];
}

template <Piece piece>
Bitboard Position::GetPiecesByType(const Player player) const
{
  return pieces_by_color_[static_cast<size_t>(player)] &
         pieces_by_type_[static_cast<size_t>(piece)];
}

template <Piece piece>
Bitboard Position::GetCastlingSquares(Castling::CastlingSide side) const
{
  static_assert(piece == Piece::kRook || piece == Piece::kKing);
  if constexpr (piece == Piece::kKing)
  {
    return castling_squares_for_king_[static_cast<size_t>(side_to_move_)]
                                     [static_cast<size_t>(side)];
  }
  if constexpr (piece == Piece::kRook)
  {
    return castling_squares_for_rook_[static_cast<size_t>(side_to_move_)]
                                     [static_cast<size_t>(side)];
  }
  assert(false);
  return {};
}

inline Bitboard Position::GetAllPawnAttacks(const Player player) const
{
  const auto us = static_cast<size_t>(player);
  const auto pawns = GetPiecesByType<Piece::kPawn>(player);
  return Shift(pawns, kPawnAttackDirections[us][0]) |
         Shift(pawns, kPawnAttackDirections[us][1]);
}

inline Piece Position::GetPiece(const BitIndex index) const
{
  return board_[index];
}

inline BitIndex Position::GetKingSquare(const Player player) const
{
  return king_position_[static_cast<size_t>(player)];
}

inline bool Position::IsUnderAttack(const BitIndex square,
                                    const Player us) const
{
  return (Attackers(square) & pieces_by_color_[static_cast<size_t>(Flip(us))])
      .Any();
}

inline bool Position::IsUnderCheck() const
{
  return IsUnderCheck(side_to_move_);
}

inline bool Position::IsUnderCheck(const Player player) const
{
  return IsUnderAttack(GetKingSquare(player), player);
}

inline const std::optional<BitIndex>& Position::GetEnCroissantSquare() const
{
  return irreversible_data_.en_croissant_square;
}

inline const std::array<std::bitset<2>, kColors>& Position::GetCastlingRights()
    const
{
  return irreversible_data_.castling_rights;
}

inline Bitboard Position::Attackers(const BitIndex square) const
{
  return GetPawnAttacks(square, Player::kWhite) &
             GetPiecesByType<Piece::kPawn>(Player::kBlack) |
         GetPawnAttacks(square, Player::kBlack) &
             GetPiecesByType<Piece::kPawn>(Player::kWhite) |
         AttackTable<Piece::kKnight>::GetAttackMap(square, GetAllPieces()) &
             pieces_by_type_[static_cast<size_t>(Piece::kKnight)] |
         AttackTable<Piece::kBishop>::GetAttackMap(square, GetAllPieces()) &
             pieces_by_type_[static_cast<size_t>(Piece::kBishop)] |
         AttackTable<Piece::kRook>::GetAttackMap(square, GetAllPieces()) &
             pieces_by_type_[static_cast<size_t>(Piece::kRook)] |
         AttackTable<Piece::kQueen>::GetAttackMap(square, GetAllPieces()) &
             pieces_by_type_[static_cast<size_t>(Piece::kQueen)] |
         AttackTable<Piece::kKing>::GetAttackMap(square, GetAllPieces()) &
             pieces_by_type_[static_cast<size_t>(Piece::kKing)];
}

inline Position::IrreversibleData Position::GetIrreversibleData() const
{
  return irreversible_data_;
}
}  // namespace SimpleChessEngine
