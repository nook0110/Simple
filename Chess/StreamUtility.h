#pragma once
#include <array>
#include <iostream>
#include <ostream>
#include <unordered_map>

#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine {
inline static std::unordered_map<char, std::pair<Piece, Player>> kPieces = {
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

inline std::ostream& operator<<(std::ostream& stream, const DefaultMove& move) {
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
inline std::ostream& operator<<(std::ostream& stream, const PawnPush& move) {
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
inline std::ostream& operator<<(std::ostream& stream, const DoublePush& move) {
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
inline std::ostream& operator<<(std::ostream& stream, const EnCroissant& move) {
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
inline std::ostream& operator<<(std::ostream& stream, const Promotion& move) {
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  constexpr std::array pieces_name = {' ', 'p', 'n', 'b', 'r', 'q', 'k'};

  std::cout << pieces_name[static_cast<size_t>(move.promoted_to)];

  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const Castling& move) {
  const auto from = GetCoordinates(move.king_from);

  static constexpr std::array kCastlingShifts = {Compass::kEast,
                                                 Compass::kWest};

  const auto shift = kCastlingShifts[static_cast<size_t>(move.side)];

  const auto to = GetCoordinates(Shift(Shift(move.king_from, shift), shift));

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
}  // namespace SimpleChessEngine