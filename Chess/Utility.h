#pragma once

#include <vector>
#include <tuple>
#include <cmath>

namespace SimpleChessEngine
{
constexpr size_t kBoardArea = 64;
constexpr int kLineSize = 8;
constexpr size_t kColors = 2;
constexpr size_t kPieceTypes =
    7;  // For Pawn, Knight, Bishop, Rook, Queen, King and Empty Square
namespace
{
    constexpr unsigned long long kFileA = 0x0101010101010101ULL;
    constexpr unsigned long long kRank1 = 0xFF;
}
constexpr std::array<Bitboard<>, kLineSize> kRankBitboard
= { kFileA, kFileA << 1, kFileA << 2, kFileA << 3, kFileA << 4, kFileA << 5, kFileA << 6, kFileA << 7 };
constexpr std::array<Bitboard<>, kLineSize> kFileBitboard
= { kRank1, kRank1 << (kLineSize * 1), kRank1 << (kLineSize * 2), kRank1 << (kLineSize* 3),
    kRank1 << (kLineSize* 4), kRank1 << (kLineSize* 5), kRank1 << (kLineSize* 6), kRank1 << (kLineSize* 7)};

[[nodiscard]] constexpr std::pair<int, int> GetCoordinates(
    const BitIndex square)
{
  return std::make_pair(static_cast<int>(square % kLineSize),
                        static_cast<int>(square / kLineSize));
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

[[nodiscard]] inline Bitboard<> GetBitboardOfSquare(const BitIndex square)
{
  return {1ull << square};
}

[[nodiscard]] inline bool IsValidShift(const BitIndex square,
                                       const Compass shift)
{
  const BitIndex new_square = square + static_cast<int>(shift);
  return IsOk(new_square) && KingDistance(square, new_square) == 1;
}
};  // namespace SimpleChessEngine
