#pragma once

#include <string>
#include <unordered_map>

#include "Piece.h"
#include "Move.h"
#include "Position.h"
#include "StreamUtility.h"
#include "Utility.h"

namespace SimpleChessEngine {

struct MoveFactory {
  Move operator()(const Position &position, const std::string &move) const;

 private:
  struct ParsedMove {
    BitIndex from;
    BitIndex to;
  };

  static ParsedMove ParseDefaultMove(const std::string &move);
};

inline Move MoveFactory::operator()(const Position &position,
                                    const std::string &move) const {
  if (move == "O-O") {
    return Move::Make<MoveType::kCastling>(
        position.GetKingSquare(position.GetSideToMove()), 
        position.GetKingSquare(position.GetSideToMove()) + 2);
  }
  if (move == "O-O-O") {
    return Move::Make<MoveType::kCastling>(
        position.GetKingSquare(position.GetSideToMove()),
        position.GetKingSquare(position.GetSideToMove()) - 2);
  }

  const auto [from, to] = ParseDefaultMove(move);

  const auto piece_to_move = position.GetPieceAt(from);

  if (piece_to_move == Piece::kKing) {
    if (!IsAdjacent(from, to)) {
      return Move::Make<MoveType::kCastling>(from, to);
    }
  }
  
  if (constexpr size_t kPromotionSize = 5; move.size() == kPromotionSize) {
    return Move::Make<MoveType::kPromotion>(from, to, kCharToPiece[move.back()].first);
  }

  if (piece_to_move != Piece::kPawn || position.GetPieceAt(to) != Piece::kNone) {
    return Move(from, to);
  }

  if (!IsAdjacent(from, to)) {
    return Move(from, to);
  }

  if (to == position.GetEnCroissantSquare()) {
    return Move::Make<MoveType::kEnPassant>(from, to);
  }

  return Move(from, to);
}

inline MoveFactory::ParsedMove MoveFactory::ParseDefaultMove(
    const std::string &move) {
  constexpr size_t first_file = 0, first_rank = 1, second_file = 2,
                   second_rank = 3;

  const ParsedMove parsed_move{
      GetSquareIndex(move[first_file] - 'a', move[first_rank] - '0' - 1),
      GetSquareIndex(move[second_file] - 'a', move[second_rank] - '0' - 1)};

  return parsed_move;
}
}  // namespace SimpleChessEngine
