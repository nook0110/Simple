#pragma once
#include <array>

#include "Bitboard.h"
#include "Hasher.h"
#include "Move.h"

constexpr size_t kBoardSize = 64;
constexpr size_t kAmountOfPlayers = 2;

namespace SimpleChessEngine
{
/**
 * \brief Enum class that represents a player.
 */
enum class Player
{
  kWhite,  //!< White player.
  kBlack   //!< Black player.
};

/**
 * \brief Enum class that represents a piece.
 */
enum class Piece
{
  kNone,    //!< No piece.
  kPawn,    //!< Pawn.
  kKnight,  //!< Knight.
  kBishop,  //!< Bishop.
  kRook,    //!< Rook.
  kQueen,   //!< Queen.
  kKing     //!< King.
};

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
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const Move& move){};

  /**
   * \brief Undoes given move.
   *
   * \param move Move to undo.
   */
  void UndoMove(const Move& move){};

  /**
   * \brief Gets hash of the position.
   *
   * \return Hash of the position.
   */
  [[nodiscard]] Hash GetHash() const { return Hash{}; };

  /**
   * \brief Gets all pieces on the board.
   *
   * \return Bitboard with all pieces on the board.
   */
  [[nodiscard]] Bitboard<kBoardSize> GetAllPieces() const
  {
    return Bitboard{pieces_[static_cast<size_t>(Player::kWhite)] |
                    pieces_[static_cast<size_t>(Player::kBlack)]};
  }

  /**
   * \brief Gets all pieces of given player.
   *
   * \param player Player to get pieces of.
   *
   * \return Bitboard with all pieces of given player.
   */
  [[nodiscard]] const Bitboard<kBoardSize>& GetPieces(Player player) const
  {
    return pieces_[static_cast<size_t>(player)];
  }

  /**
   * \brief Gets piece on given square.
   *
   * \param index Square to get piece from.
   *
   * \return Piece on given square.
   */
  [[nodiscard]] Piece GetPiece(BitIndex index) const { return Piece{}; }

  /**
   * \brief Gets side to move.
   *
   * \return Side to move.
   */
  [[nodiscard]] Player GetSideToMove() const { return side_to_move_; }

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

 private:
  Player side_to_move_{};  //!< Whose side to move.

  std::array<Bitboard<kBoardSize>, kAmountOfPlayers>
      pieces_;  //!< Bitboard of pieces for each player

  std::array<Piece, kBoardSize> board_{};  //!< Current position of pieces
};
}  // namespace SimpleChessEngine
