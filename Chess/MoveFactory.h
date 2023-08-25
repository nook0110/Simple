#pragma once

#include "Move.h"
#include "Position.h"
#include "StreamUtility.h"

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

  constexpr size_t kPromotionSize = 5;
  if (move.size() == 5)
  {
    return Promotion{{from, to, position.GetPiece(to)},
                     kPieces[move.back()].first};
  }

  if (position.GetPiece(from) != Piece::kPawn)
  {
    return DefaultMove{from, to, position.GetPiece(to)};
  }

  if (constexpr BitIndex kDoublePushDelta = 16;
      std::abs(from - to) == kDoublePushDelta)
  {
    return DoublePush{from, to};
  }

  if (position.GetPiece(to) == Piece::kNone)
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

  ParsedMove parsed_move{};

  parsed_move.from =
      GetSquare(move[first_file] - 'a', move[first_rank] - '0' - 1);
  parsed_move.to =
      GetSquare(move[second_file] - 'a', move[second_rank] - '0' - 1);

  return parsed_move;
}
}  // namespace SimpleChessEngine
