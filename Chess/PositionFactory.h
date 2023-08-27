#pragma once
#include <unordered_map>

#include "Position.h"
#include "StreamUtility.h"

namespace SimpleChessEngine
{
struct PositionFactory
{
  Position operator()(
      const std::string& fen =
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

  Position operator()(std::stringstream stream);
};

inline Position PositionFactory::operator()(const std::string& fen)
{
  Position position;

  static constexpr size_t kBoardSize = 8;

  auto current = fen.begin();

  BitIndex white_king{}, black_king{};

  for (int row = kBoardSize - 1; row >= 0; --row)
  {
    size_t column = 0;
    while (column < kBoardSize)
    {
      if (std::isdigit(*current))
      {
        // convert char to number
        column += *current - '0';

        ++current;
        continue;
      }

      const auto& [piece, color] = kPieces[*current];
      auto square = row * kBoardSize + column;
      position.PlacePiece(square, piece, color);

      if (piece == Piece::kKing)
      {
        if (color == Player::kWhite)
          white_king = square;
        else
          black_king = square;
      }

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

  position.SetKingPositions(white_king, black_king);

  return position;
}

inline Position PositionFactory::operator()(std::stringstream stream) {}
}  // namespace SimpleChessEngine
