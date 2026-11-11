#pragma once

#include "../../Position.h"
#include "bitboard.h"
#include "types.h"

namespace Stockfish {
class Position {
 public:
  explicit Position(const SimpleChessEngine::Position& position)
      : position_(position) {}

  Bitboard pieces() const {
    return static_cast<Bitboard>(position_.GetAllPieces());
  }
  Bitboard pieces(Color color) const {
    return static_cast<Bitboard>(
        position_.GetPieces(static_cast<SimpleChessEngine::Player>(color)));
  }
  template <typename... Types>
  Bitboard pieces(PieceType first, Types... rest) const {
    const auto both =
        position_.GetPiecesOf(SimpleChessEngine::Player::kWhite,
                              static_cast<SimpleChessEngine::Piece>(first)) |
        position_.GetPiecesOf(SimpleChessEngine::Player::kBlack,
                              static_cast<SimpleChessEngine::Piece>(first));
    if constexpr (sizeof...(rest))
      return static_cast<Bitboard>(both) | pieces(rest...);
    return static_cast<Bitboard>(both);
  }
  template <typename... Types>
  Bitboard pieces(Color color, PieceType first, Types... rest) const {
    return pieces(color) & pieces(first, rest...);
  }
  template <PieceType Type>
  Square square(Color color) const {
    static_assert(Type == KING);
    return static_cast<Square>(
        position_.GetKingSquare(static_cast<SimpleChessEngine::Player>(color)));
  }
  template <PieceType Type>
  int count() const {
    if constexpr (Type == ALL_PIECES)
      return popcount(pieces());
    else
      return popcount(pieces(Type));
  }
  Piece piece_on(Square square) const {
    const auto piece = position_.GetPieceAt(static_cast<BitIndex>(square));
    if (piece == SimpleChessEngine::Piece::kNone) return NO_PIECE;
    return make_piece(
        position_.GetPieces(SimpleChessEngine::Player::kWhite).Test(square)
            ? WHITE
            : BLACK,
        static_cast<PieceType>(piece));
  }
  std::array<Piece, SQUARE_NB> piece_array() const {
    std::array<Piece, SQUARE_NB> board{};
    for (Bitboard occupied = pieces(); occupied;) {
      const Square sq = pop_lsb(occupied);
      board[sq] = piece_on(sq);
    }
    return board;
  }
  Color side_to_move() const {
    return static_cast<Color>(position_.GetSideToMove());
  }

  template <bool ComputeRay = true>
  void update_piece_threats(Piece pc, bool putPiece, Square s,
                            DirtyThreats* dts,
                            Bitboard noRaysContaining = -1ULL) const;

 private:
  const SimpleChessEngine::Position& position_;
};
}  // namespace Stockfish
