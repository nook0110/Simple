#pragma once
#include <sstream>
#include <unordered_map>

#include "Position.h"
#include "StreamUtility.h"

namespace SimpleChessEngine
{
struct PositionFactory
{
  Position operator()(
      const std::string& fen =
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") const;

  Position operator()(std::stringstream stream) const;
};

inline Position PositionFactory::operator()(const std::string& fen) const
{
  std::stringstream fen_stream;

  fen_stream << fen;

  Position position;

  static constexpr size_t kBoardSize = 8;

  std::string board;

  fen_stream >> board;

  auto current = board.begin();

  std::array<BitIndex, kColors> kings{};
  std::array<std::array<BitIndex, 2>, kColors> rooks{};

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
      auto square = static_cast<BitIndex>(row * kBoardSize + column);
      position.PlacePiece(square, piece, color);

      const auto us = static_cast<size_t>(color);

      if (piece == Piece::kKing)
      {
        kings[us] = square;
      }

      if (piece == Piece::kRook)
      {
        rooks[us][!kings[us]] = square;
      }

      assert(piece != Piece::kNone);

      ++column;
      ++current;
    }

    // skip '/'
    if (current != board.end()) ++current;
  }

  std::string side_to_move;
  fen_stream >> side_to_move;

  if (side_to_move == "w")
    position.SetSideToMove(Player::kWhite);
  else
    position.SetSideToMove(Player::kBlack);

  // TODO: Attributes
  std::string castling_rights_string;
  fen_stream >> castling_rights_string;

  std::array<std::bitset<2>, 2> castling_rights;

  for (auto castling_right : castling_rights_string)
  {
    switch (castling_right)
    {
      case 'K':
        castling_rights[static_cast<size_t>(Player::kWhite)] |=
            static_cast<size_t>(CastlingRights::k00);
        break;
      case 'k':
        castling_rights[static_cast<size_t>(Player::kBlack)] |=
            static_cast<size_t>(CastlingRights::k00);
        break;
      case 'Q':
        castling_rights[static_cast<size_t>(Player::kWhite)] |=
            static_cast<size_t>(CastlingRights::k000);
        break;
      case 'q':
        castling_rights[static_cast<size_t>(Player::kBlack)] |=
            static_cast<size_t>(CastlingRights::k000);
        break;
      default:
        assert(false);
    }
  }

  position.SetCastlingRights(castling_rights);

  position.SetKingPositions(kings);
  position.SetRookPositions(rooks);
  std::array<std::array<Bitboard, 2>, kColors> cs_king = {
      {{rook_between[kings[0]][kKingCastlingDestination[0][0]],
        rook_between[kings[0]][kKingCastlingDestination[0][1]]},
       {rook_between[kings[1]][kKingCastlingDestination[1][0]],
        rook_between[kings[1]][kKingCastlingDestination[1][1]]}}};
  std::array<std::array<Bitboard, 2>, kColors> cs_rook = {
      {{rook_between[rooks[0][0]][kRookCastlingDestination[0][0]],
        rook_between[rooks[0][1]][kRookCastlingDestination[0][1]]},
       {rook_between[rooks[1][0]][kRookCastlingDestination[1][0]],
        rook_between[rooks[1][1]][kRookCastlingDestination[1][1]]}}};
  position.SetCastlingSquares(cs_king, cs_rook);

  return position;
}

inline Position PositionFactory::operator()(std::stringstream stream) const
{
  return Position{};
}
}  // namespace SimpleChessEngine
