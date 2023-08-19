#pragma once
#include <unordered_map>

#include "Position.h"

namespace SimpleChessEngine
{
struct PositionFactory
{
  Position operator()(
      const std::string& fen =
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
};

inline Position PositionFactory::operator()(const std::string& fen)
{
  Position position;

  constexpr size_t board_size = 8;

  std::unordered_map<char, std::pair<Piece, Player>> pieces = {
      {'p', {Piece::kPawn, Player::kBlack}},
      {'P', {Piece::kPawn, Player::kWhite}},
      {'n', {Piece::kKnight, Player::kBlack}},
      {'N', {Piece::kKnight, Player::kWhite}},
      {'b', {Piece::kBishop, Player::kBlack}},
      {'B', {Piece::kBishop, Player::kWhite}},
      {'r', {Piece::kRook, Player::kBlack}},
      {'R', {Piece::kRook, Player::kWhite}},
      {'q', {Piece::kQueen, Player::kBlack}},
      {'Q', {Piece::kQueen, Player::kWhite}},
      {'k', {Piece::kKing, Player::kBlack}},
      {'K', {Piece::kKing, Player::kWhite}}};

  auto current = fen.begin();

  for (int row = board_size - 1; row >= 0; --row)
  {
    size_t column = 0;
    while (column < board_size)
    {
      if (std::isdigit(*current))
      {
        // convert char to number
        column += *current - '0';

        ++current;
        continue;
      }

      const auto& [piece, color] = pieces[*current];
      position.PlacePiece(row * board_size + column, piece, color);

      assert(piece != Piece::kNone);

      ++column;
      ++current;
    }

    // skip '/'
    ++current;
  }

  if (*current == 'w')
    position.SetSideToMove(Player::kWhite);
  else
    position.SetSideToMove(Player::kBlack);

  // TODO: Attributes

  return position;
}
}  // namespace SimpleChessEngine
