#pragma once
#include <array>
#include <cassert>

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
    pieces_by_color_[static_cast<size_t>(color)].set(square);
    pieces_by_type_[static_cast<size_t>(piece)].set(square);
    // TODO: hash update
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
    pieces_by_type_[static_cast<size_t>(piece)].reset(square);
    pieces_by_color_[static_cast<size_t>(color)].reset(square);
    board_[square] = Piece::kNone;
    // TODO: hash update
  }

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const Move& move);

  /**
   * \brief Undoes given move.
   *
   * \param move Move to undo.
   */
  void UndoMove(const Move& move);

  /**
   * \brief Gets hash of the position.
   *
   * \return Hash of the position.
   */
  [[nodiscard]] Hash GetHash() const { return Hash{}; }

  /**
   * \brief Gets all pieces on the board.
   *
   * \return Bitboard with all pieces on the board.
   */
  [[nodiscard]] Bitboard<kBoardArea> GetAllPieces() const
  {
    return pieces_by_color_[static_cast<size_t>(Player::kWhite)] |
           pieces_by_color_[static_cast<size_t>(Player::kBlack)];
  }

  /**
   * \brief Gets all pieces of given player.
   *
   * \param player Player to get pieces of.
   *
   * \return Bitboard with all pieces of given player.
   */
  [[nodiscard]] const Bitboard<kBoardArea>& GetPieces(Player player) const
  {
    return pieces_by_color_[static_cast<size_t>(player)];
  }

  /**
   * \brief Gets piece on given square.
   *
   * \param index Square to get piece from.
   *
   * \return Piece on given square.
   */
  [[nodiscard]] Piece GetPiece(const BitIndex index) const
  {
    return board_[index];
  }

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

  /**
   * \brief Checks if current player is under check.
   *
   * \return True if current player is under check, false otherwise.
   */
  [[nodiscard]] bool IsUnderCheck() const
  {
    return IsUnderCheck(side_to_move_);
  }

  /**
   * \brief Checks if given player is under check.
   *
   * \return True if given player is under check, false otherwise.
   */
  [[nodiscard]] bool IsUnderCheck(Player player) const { return false; }

  /**
   * \brief Default operator==()
   *
   * \param other Other position to compare with.
   *
   * \return True if positions are the same, false otherwise.
   */
  bool operator==(const Position& other) const = default;

 private:
  Player side_to_move_{};  //!< Whose side to move.

  std::array<Bitboard<kBoardArea>, kPieceTypes>
      pieces_by_type_;  //!< Bitboard of pieces of certain type
  std::array<Bitboard<kBoardArea>, kColors>
      pieces_by_color_;  //!< Bitboard of pieces for each player

  std::array<Piece, kBoardArea> board_{};  //!< Current position of pieces
};
}  // namespace SimpleChessEngine
