#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

namespace SimpleChessEngine
{
inline constexpr size_t kBoardArea = 64;
const Bitboard kEmptyBoard = Bitboard{};
constexpr int kLineSize = 8;
constexpr size_t kColors = 2;
constexpr size_t kPieceTypes =
    7;  // For Pawn, Knight, Bishop, Rook, Queen, King and Empty Square
namespace
{
    constexpr Bitboard kFileA{ 0x0101010101010101ULL };
    constexpr Bitboard kRank1{ 0xFF };
}  // namespace
constexpr std::array<Bitboard, kLineSize> kRankBB = {
    kRank1,
    kRank1 << (kLineSize * 1),
    kRank1 << (kLineSize * 2),
    kRank1 << (kLineSize * 3),
    kRank1 << (kLineSize * 4),
    kRank1 << (kLineSize * 5),
    kRank1 << (kLineSize * 6),
    kRank1 << (kLineSize * 7)};
constexpr std::array<Bitboard, kLineSize> kFileBB = {
    kFileA,      kFileA << 1, kFileA << 2, kFileA << 3,
    kFileA << 4, kFileA << 5, kFileA << 6, kFileA << 7};

/*
constexpr std::array<Bitboard, kColors> kCloseRanks = { kRankBB[2].to_ullong() | kRankBB[3].to_ullong(), 
                                                        kRankBB[5].to_ullong() | kRankBB[4].to_ullong() };
                                                        */

/*
constexpr std::array<std::array<Bitboard, kLineSize>, kColors> kDoubleMoveSpan =
{};
*/

using Rank = int;
using File = int;

[[nodiscard]] constexpr std::pair<File, Rank> GetCoordinates(
    const BitIndex square)
{
  return std::make_pair(static_cast<int>(square % kLineSize),
                        static_cast<int>(square / kLineSize));
}

[[nodiscard]] constexpr BitIndex GetSquare(File file, Rank rank)
{
  assert(file >= 0 && file < kLineSize);
  assert(rank >= 0 && rank < kLineSize);
  return rank << 3 | file;
}

[[nodiscard]] inline int ManhattanDistance(const BitIndex first,
                                           const BitIndex second)
{
  const auto [x_first, y_first] = GetCoordinates(first);
  const auto [x_second, y_second] = GetCoordinates(second);
  return std::abs(x_first - x_second) + std::abs(y_first - y_second);
}

[[nodiscard]] inline int KingDistance(const BitIndex first,
                                      const BitIndex second)
{
  const auto [x_first, y_first] = GetCoordinates(first);
  const auto [x_second, y_second] = GetCoordinates(second);
  return std::max(std::abs(x_first - x_second), std::abs(y_first - y_second));
}

enum class Compass
{
  kNorth = kLineSize,
  kWest = -1,
  kSouth = -kLineSize,
  kEast = +1,
  kNorthWest = kNorth + kWest,
  kSouthWest = kSouth + kWest,
  kSouthEast = kSouth + kEast,
  kNorthEast = kNorth + kEast
};

[[nodiscard]] inline bool IsOk(const BitIndex square)
{
  return square >= 0 && square < kBoardArea;
}

[[nodiscard]] inline Bitboard GetBitboardOfSquare(const BitIndex square)
{
  return Bitboard{1ull << square};
}

[[nodiscard]] inline bool IsValidShift(const BitIndex square,
                                       const Compass shift)
{
  const BitIndex new_square = square + static_cast<int>(shift);
  return IsOk(new_square) && KingDistance(square, new_square) == 1;
}

[[nodiscard]] inline constexpr bool IsSlidingPiece(const Piece piece)
{
  return piece == Piece::kBishop || piece == Piece::kRook ||
         piece == Piece::kQueen;
}

[[nodiscard]] inline constexpr bool IsWeakSlidingPiece(const Piece piece)
{
  return IsSlidingPiece(piece) && piece != Piece::kQueen;
}

[[nodiscard]] inline std::string DrawBitboard(const Bitboard b)
{
  std::string s = "+---+---+---+---+---+---+---+---+\n";

  for (Rank r = 7; r >= 0; --r)
  {
    for (File f = 0; f <= 7; ++f)
    {
      s += (b & GetBitboardOfSquare(GetSquare(f, r))).any() ? "| X " : "|   ";
    }

    s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
  }
  s += "  a   b   c   d   e   f   g   h\n";

  return s;
}
};  // namespace SimpleChessEngine
