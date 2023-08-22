#pragma once

#include <cmath>
#include <string>

namespace SimpleChessEngine
{
inline constexpr size_t kBoardArea = 64;
constexpr Bitboard kEmptyBoard = Bitboard{};
constexpr int kLineSize = 8;
constexpr size_t kColors = 2;
constexpr size_t kPieceTypes =
    7;  // For Pawn, Knight, Bishop, Rook, Queen, King and Empty Square

namespace  // TODO: Name namespace
{
constexpr Bitboard kFileA{0x0101010101010101ULL};
constexpr Bitboard kRank1{0xFF};
}  // namespace

constexpr std::array kRankBB = {kRank1,
                                kRank1 << static_cast<size_t>(kLineSize) * 1,
                                kRank1 << static_cast<size_t>(kLineSize) * 2,
                                kRank1 << static_cast<size_t>(kLineSize) * 3,
                                kRank1 << static_cast<size_t>(kLineSize) * 4,
                                kRank1 << static_cast<size_t>(kLineSize) * 5,
                                kRank1 << static_cast<size_t>(kLineSize) * 6,
                                kRank1 << static_cast<size_t>(kLineSize) * 7};
constexpr std::array kFileBB = {kFileA,      kFileA << 1, kFileA << 2,
                                kFileA << 3, kFileA << 4, kFileA << 5,
                                kFileA << 6, kFileA << 7};
constexpr std::array kCloseRanks = {kRankBB[2] | kRankBB[3],
                                    kRankBB[5] | kRankBB[4]};

constexpr std::array<std::array<Bitboard, kLineSize>, kColors> kDoubleMoveSpan =
    {{{kCloseRanks[0] & kFileBB[0], kCloseRanks[0] & kFileBB[1],
       kCloseRanks[0] & kFileBB[2], kCloseRanks[0] & kFileBB[3],
       kCloseRanks[0] & kFileBB[4], kCloseRanks[0] & kFileBB[5],
       kCloseRanks[0] & kFileBB[6], kCloseRanks[0] & kFileBB[7]},
      {kCloseRanks[1] & kFileBB[0], kCloseRanks[1] & kFileBB[1],
       kCloseRanks[1] & kFileBB[2], kCloseRanks[1] & kFileBB[3],
       kCloseRanks[1] & kFileBB[4], kCloseRanks[1] & kFileBB[5],
       kCloseRanks[1] & kFileBB[6], kCloseRanks[1] & kFileBB[7]}}};

using Rank = int;
using File = int;

[[nodiscard]] constexpr std::pair<File, Rank> GetCoordinates(
    const BitIndex square)
{
  return std::make_pair(static_cast<int>(square % kLineSize),
                        static_cast<int>(square / kLineSize));
}

[[nodiscard]] constexpr BitIndex GetSquare(const File file, const Rank rank)
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

constexpr std::array kPawnMoveDirection = {Compass::kNorth, Compass::kSouth};

[[nodiscard]] inline bool IsOk(const BitIndex square)
{
  return square < kBoardArea;
}

[[nodiscard]] inline Bitboard GetBitboardOfSquare(const BitIndex square)
{
  return Bitboard{1ull << square};
}

[[nodiscard]] inline bool IsAdjacent(const BitIndex sq_first,
                                     const BitIndex sq_second)
{
  return KingDistance(sq_first, sq_second) == 1;
}

[[nodiscard]] inline Bitboard GetShiftIfValid(const BitIndex square,
                                              const Compass shift)
{
  const BitIndex new_square = square + static_cast<int>(shift);
  return IsOk(new_square) && IsAdjacent(square, new_square)
             ? GetBitboardOfSquare(new_square)
             : Bitboard{};
}

[[nodiscard]] inline Bitboard DoShiftIfValid(BitIndex& square,
                                             const Compass shift)
{
  BitIndex copy = square;
  square += static_cast<int>(shift);
  return IsOk(square) && IsAdjacent(copy, square) ? GetBitboardOfSquare(square)
                                                  : Bitboard{};
}

[[nodiscard]] inline Bitboard GetPawnAttacks(const BitIndex square,
                                             const Player side)
{
  Bitboard res{};
  static constexpr std::array<std::array<Compass, 2>, kColors>
      kPawnAttackDirections = {{{Compass::kNorthWest, Compass::kNorthEast},
                                {Compass::kSouthWest, Compass::kSouthEast}}};
  for (auto step : kPawnAttackDirections[static_cast<size_t>(side)])
  {
    res |= GetShiftIfValid(square, step);
  }
  return res;
}

[[nodiscard]] constexpr bool IsSlidingPiece(const Piece piece)
{
  return piece == Piece::kBishop || piece == Piece::kRook ||
         piece == Piece::kQueen;
}

[[nodiscard]] constexpr bool IsWeakSlidingPiece(const Piece piece)
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
      s += (b & GetBitboardOfSquare(GetSquare(f, r))).Any() ? "| X " : "|   ";
    }

    s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
  }
  s += "  a   b   c   d   e   f   g   h\n";

  return s;
}
};  // namespace SimpleChessEngine
