#pragma once
#include <array>
#include <iostream>
#include <ostream>
#include <unordered_map>

#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine {
inline static std::unordered_map<char, std::pair<Piece, Player>> kCharToPiece =
    {{'p', {Piece::kPawn, Player::kBlack}},
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

inline constexpr std::array kPiecesChars = {' ', 'p', 'n', 'b', 'r', 'q', 'k'};

inline char GetPieceChar(Piece piece, Player color) {
  char piece_name = kPiecesChars[static_cast<size_t>(piece)];
  return color == Player::kWhite ? toupper(piece_name) : piece_name;
}

inline std::ostream& PrintFile(const File file,
                               std::ostream& stream = std::cout) {
  stream << static_cast<char>(file + 'a');
  return stream;
}

inline std::ostream& PrintRank(const Rank rank,
                               std::ostream& stream = std::cout) {
  stream << rank + 1;
  return stream;
}

inline std::ostream& PrintCoordinates(const Coordinates coordinates,
                                      std::ostream& stream) {
  PrintFile(coordinates.first, stream);
  PrintRank(coordinates.second, stream);

  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const Move& move) {
  const auto from = GetCoordinates(move.From());
  const auto to = GetCoordinates(move.To());

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  if (move.IsPromotion()) {
    stream << kPiecesChars[static_cast<size_t>(move.PromotionPiece())];
  }

  return stream;
}

}  // namespace SimpleChessEngine