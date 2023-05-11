#pragma once
#include <array>

#include "Bitboard.h"
#include "Hasher.h"
#include "Move.h"

constexpr size_t kBoardSize = 64;
constexpr size_t kAmountOfPlayers = 2;

namespace SimpleChessEngine
{
enum class Player
{
  kWhite,
  kBlack
};

enum class Piece
{
  kPawn,
  kKnight,
  kBishop,
  kRook,
  kQueen,
  kKing
};

class Position
{
 public:
  void DoMove(const Move& move){};
  void UndoMove(const Move& move){};

  [[nodiscard]] Hash GetHash() const { return Hash{}; };

  [[nodiscard]] const Bitboard<kBoardSize>& GetAllPieces() const
  {
    return Bitboard<>{pieces_[static_cast<size_t>(Player::kWhite)] |
                      pieces_[static_cast<size_t>(Player::kBlack)]};
  }

  [[nodiscard]] const Bitboard<kBoardSize>& GetPieces(Player player) const
  {
    return pieces_[static_cast<size_t>(player)];
  }

  [[nodiscard]] Piece GetPiece(BitIndex index) const { return Piece{}; }

  [[nodiscard]] Player GetSideToMove() const { return side_to_move_; }

  [[nodiscard]] bool IsUnderCheck() const
  {
    return IsUnderCheck(side_to_move_);
  }
  [[nodiscard]] bool IsUnderCheck(Player player) const { return false; }

 private:
  Player side_to_move_{};

  std::array<Bitboard<kBoardSize>, kAmountOfPlayers> pieces_;

  std::array<Piece, kBoardSize> board_;
};
}  // namespace SimpleChessEngine
