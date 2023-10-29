#pragma once

#include "Move.h"
#include "Position.h"
#include "StreamUtility.h"
#include "Utility.h"

namespace SimpleChessEngine
{
struct MoveFactory
{
  Move operator()(const Position& position, const std::string& move) const;

 private:
  struct ParsedMove
  {
    BitIndex from;
    BitIndex to;
  };

  static ParsedMove ParseDefaultMove(const std::string& move);
};

inline Move MoveFactory::operator()(const Position& position,
                                    const std::string& move) const
{
  if (move == "O-O")
  {
    return Castling{Castling::CastlingSide::k00,
                    position.GetKingSquare(position.GetSideToMove())};
  }
  if (move == "O-O-O")
  {
    return Castling{Castling::CastlingSide::k000,
                    position.GetKingSquare(position.GetSideToMove())};
  }

  const auto [from, to] = ParseDefaultMove(move);

  const auto piece_to_move = position.GetPiece(from);

  if (piece_to_move == Piece::kKing)
  {
    if (!IsAdjacent(from, to))
    {
      static std::unordered_map<File, Castling::CastlingSide> castling_file = {
          {2, Castling::CastlingSide::k000}, {6, Castling::CastlingSide::k00}};
      static std::unordered_map<File, File> rook_from_file = {{1, 0}, {6, 7}};

      auto [king_file, king_rank] = GetCoordinates(to);

      return Castling{castling_file[king_file], from,
                      GetSquare(rook_from_file[king_file], king_rank)};
    }
  }
  if (constexpr size_t kPromotionSize = 5; move.size() == kPromotionSize)
  {
    return Promotion{{from, to, position.GetPiece(to)},
                     kPieces[move.back()].first};
  }

  if (piece_to_move != Piece::kPawn)
  {
    return DefaultMove{from, to, position.GetPiece(to)};
  }

  if (!IsAdjacent(from, to))
  {
    return DoublePush{from, to};
  }

  if (to == position.GetEnCroissantSquare())
  {
    return EnCroissant{from, to};
  }

  return DefaultMove{from, to, position.GetPiece(to)};
}

inline MoveFactory::ParsedMove MoveFactory::ParseDefaultMove(
    const std::string& move)
{
  constexpr size_t first_file = 0, first_rank = 1, second_file = 2,
                   second_rank = 3;

  const ParsedMove parsed_move{
      GetSquare(move[first_file] - 'a', move[first_rank] - '0' - 1),
      GetSquare(move[second_file] - 'a', move[second_rank] - '0' - 1)};

  return parsed_move;
}
}  // namespace SimpleChessEngine
