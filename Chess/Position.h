#pragma once
#include <array>

#include "BitBoard.h"
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

  [[nodiscard]] const BitBoard<kBoardSize>& GetPieces() const
  {
    return pieces_[static_cast<size_t>(sideToMove_)];
  }

  [[nodiscard]] Piece GetPiece(BitIndex index) const { return Piece{}; }

  [[nodiscard]] Player GetSideToMove() const { return sideToMove_; }

 private:
  Player sideToMove_;

  std::array<BitBoard<kBoardSize>, kAmountOfPlayers> pieces_;

  std::array<Piece, kBoardSize> board_;
};
}  // namespace SimpleChessEngine
