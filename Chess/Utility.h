#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "BitBoard.h"
#include "Piece.h"
#include "Player.h"

namespace SimpleChessEngine {
using Depth = std::uint8_t;
using Age = std::uint16_t;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

constexpr size_t kBoardArea = 64;
constexpr Bitboard kEmptyBoard = Bitboard{};
constexpr std::int8_t kLineSize = 8;
constexpr size_t kColors = 2;
constexpr size_t kPieceTypes =
    7;  // For Pawn, Knight, Bishop, Rook, Queen, King and Empty Square
constexpr size_t kMaxSearchPly = 1 << 6;

constexpr Bitboard kFileA{0x0101010101010101ULL};
constexpr Bitboard kRank1{0xFF};

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

constexpr std::array<std::array<BitIndex, 2>, kColors>
    kKingCastlingDestination = {{{6, 2}, {62, 58}}};
constexpr std::array<std::array<BitIndex, 2>, kColors>
    kRookCastlingDestination = {{{5, 3}, {61, 59}}};

using Rank = std::int8_t;
using File = std::int8_t;

using Coordinates = std::pair<File, Rank>;

[[nodiscard]] constexpr Coordinates GetCoordinates(const BitIndex square) {
  return std::make_pair(square % kLineSize, square / kLineSize);
}

[[nodiscard]] constexpr BitIndex GetSquareIndex(const File file, const Rank rank) {
  assert(file >= 0 && file < kLineSize);
  assert(rank >= 0 && rank < kLineSize);
  return rank << 3 | file;
}

[[nodiscard]] constexpr int KingDistance(const BitIndex first,
                                         const BitIndex second) {
  const auto [x_first, y_first] = GetCoordinates(first);
  const auto [x_second, y_second] = GetCoordinates(second);
  const int file_distance =
      x_first > x_second ? x_first - x_second : x_second - x_first;
  const int rank_distance =
      y_first > y_second ? y_first - y_second : y_second - y_first;
  return std::max(file_distance, rank_distance);
}

enum class CastlingRights : std::uint8_t {
  kNone,
  k00 = 1,
  k000 = 2,
  kAll = k00 | k000
};

constexpr std::array kCastlingRightsForSide = {CastlingRights::k00,
                                               CastlingRights::k000};

enum class Compass : std::int8_t {
  kNorth = kLineSize,
  kWest = -1,
  kSouth = -kLineSize,
  kEast = +1,
  kNorthWest = kNorth + kWest,
  kSouthWest = kSouth + kWest,
  kSouthEast = kSouth + kEast,
  kNorthEast = kNorth + kEast
};

constexpr Compass Invert(const Compass direction) {
  return static_cast<Compass>(-static_cast<std::int8_t>(direction));
}

[[nodiscard]] constexpr Bitboard Shift(const Bitboard bb,
                                       const Compass direction) {
  switch (direction) {
    case Compass::kNorth:
      return bb << kLineSize;
    case Compass::kSouth:
      return bb >> kLineSize;
    case Compass::kEast:
      return (bb & ~kFileBB.back()) << 1;
    case Compass::kWest:
      return (bb & ~kFileBB.front()) >> 1;
    case Compass::kNorthEast:
      return Shift(Shift(bb, Compass::kNorth), Compass::kEast);
    case Compass::kNorthWest:
      return Shift(Shift(bb, Compass::kNorth), Compass::kWest);
    case Compass::kSouthEast:
      return Shift(Shift(bb, Compass::kSouth), Compass::kEast);
    case Compass::kSouthWest:
      return Shift(Shift(bb, Compass::kSouth), Compass::kWest);
    default:
      return Bitboard{};
  }
}

[[nodiscard]] constexpr BitIndex Shift(const BitIndex square,
                                       const Compass direction) {
  return square + static_cast<BitIndex>(direction);
}

constexpr std::array kPawnMoveDirection = {Compass::kNorth, Compass::kSouth};
constexpr std::array<std::array<Compass, 2>, kColors> kPawnAttackDirections = {
    {{Compass::kNorthWest, Compass::kNorthEast},
     {Compass::kSouthWest, Compass::kSouthEast}}};

struct PawnSpanTables {
  std::array<std::array<Bitboard, kBoardArea>, kColors> front{};
  std::array<std::array<Bitboard, kBoardArea>, kColors> rear{};
  std::array<std::array<Bitboard, kBoardArea>, kColors> passed{};
  std::array<Bitboard, kBoardArea> adjacent_files{};
};

consteval PawnSpanTables MakePawnSpanTables() {
  PawnSpanTables tables{};
  for (size_t side = 0; side < kColors; ++side) {
    const int forward =
        side == static_cast<size_t>(Player::kWhite) ? 1 : -1;
    for (size_t square = 0; square < kBoardArea; ++square) {
      const int file = static_cast<int>(square % kLineSize);
      const int rank = static_cast<int>(square / kLineSize);
      std::uint64_t front = 0;
      std::uint64_t rear = 0;
      std::uint64_t passed = 0;
      std::uint64_t adjacent_files = 0;

      for (int target_rank = 0; target_rank < kLineSize; ++target_rank) {
        const bool is_front = (target_rank - rank) * forward > 0;
        const bool is_rear = (target_rank - rank) * forward < 0;
        for (int target_file = 0; target_file < kLineSize; ++target_file) {
          const int file_difference = target_file > file
                                          ? target_file - file
                                          : file - target_file;
          const auto target = static_cast<size_t>(target_rank * kLineSize +
                                                  target_file);
          const auto target_bit = std::uint64_t{1} << target;
          if (target_file == file && is_front) front |= target_bit;
          if (target_file == file && is_rear) rear |= target_bit;
          if (file_difference <= 1 && is_front) {
            passed |= target_bit;
          }
          if (file_difference == 1) adjacent_files |= target_bit;
        }
      }

      tables.front[side][square] = Bitboard{front};
      tables.rear[side][square] = Bitboard{rear};
      tables.passed[side][square] = Bitboard{passed};
      if (side == 0) {
        tables.adjacent_files[square] = Bitboard{adjacent_files};
      }
    }
  }
  return tables;
}

inline constexpr PawnSpanTables kPawnSpans = MakePawnSpanTables();

[[nodiscard]] constexpr bool IsOk(const BitIndex square) {
  return 0 <= square && square < static_cast<BitIndex>(kBoardArea);
}

[[nodiscard]] constexpr Bitboard SingleSquare(const BitIndex square_index) {
  return Bitboard{1ull << square_index};
}

[[nodiscard]] constexpr bool IsAdjacent(const BitIndex sq_first,
                                        const BitIndex sq_second) {
  return KingDistance(sq_first, sq_second) == 1;
}

[[nodiscard]] constexpr bool IsShiftValid(const BitIndex shifted_square,
                                          const BitIndex square) {
  return IsOk(shifted_square) && IsAdjacent(square, shifted_square);
}

[[nodiscard]] constexpr std::optional<Bitboard> GetShiftIfValid(
    const BitIndex square, const Compass shift) {
  const BitIndex new_square = Shift(square, shift);
  return IsShiftValid(new_square, square)
             ? std::optional{SingleSquare(new_square)}
             : std::nullopt;
}

[[nodiscard]] constexpr std::optional<Bitboard> DoShiftIfValid(
    BitIndex& square, const Compass shift) {
  const BitIndex copy = square;
  square += static_cast<int>(shift);
  return IsShiftValid(square, copy) ? std::optional{SingleSquare(square)}
                                    : std::nullopt;
}

[[nodiscard]] constexpr bool IsSlidingPiece(const Piece piece) {
  return piece == Piece::kBishop || piece == Piece::kRook ||
         piece == Piece::kQueen;
}

[[nodiscard]] constexpr bool IsWeakSlidingPiece(const Piece piece) {
  return IsSlidingPiece(piece) && piece != Piece::kQueen;
}

[[nodiscard]] constexpr bool IsDiagonalAttacker(const Piece piece) {
  return piece == Piece::kPawn || piece == Piece::kBishop ||
         piece == Piece::kQueen;
}

[[nodiscard]] constexpr bool IsStraightAttacker(const Piece piece) {
  return piece == Piece::kRook || piece == Piece::kQueen;
}

using SquareTable = std::array<std::array<Bitboard, kBoardArea>, kBoardArea>;

struct GeometryTables {
  SquareTable between{};
  SquareTable ray{};
};

extern const GeometryTables kGeometry;

[[nodiscard]] inline Bitboard Between(const BitIndex from, const BitIndex to) {
  return kGeometry.between[from][to];
}

[[nodiscard]] inline Bitboard RookBetween(const BitIndex from,
                                          const BitIndex to) {
  const auto [from_file, from_rank] = GetCoordinates(from);
  const auto [to_file, to_rank] = GetCoordinates(to);
  if (from_file != to_file && from_rank != to_rank) return kEmptyBoard;
  return Between(from, to);
}

[[nodiscard]] inline Bitboard Ray(const BitIndex from, const BitIndex to) {
  return kGeometry.ray[from][to];
}

consteval auto MakePawnAttacks() {
  std::array<std::array<Bitboard, kBoardArea>, kColors> attacks{};
  for (size_t color = 0; color < kColors; ++color) {
    for (BitIndex square = 0; square < static_cast<BitIndex>(kBoardArea);
         ++square) {
      for (const auto direction : kPawnAttackDirections[color]) {
        attacks[color][square] |=
            GetShiftIfValid(square, direction).value_or(Bitboard{});
      }
    }
  }
  return attacks;
}

inline constexpr auto kPawnAttacks = MakePawnAttacks();

[[nodiscard]] constexpr Bitboard GetPawnAttacks(const BitIndex square,
                                                const Player side) {
  return kPawnAttacks[static_cast<size_t>(side)][square];
}

[[nodiscard]] inline std::string DrawBitboard(const Bitboard b) {
  std::string s = "+---+---+---+---+---+---+---+---+\n";

  for (Rank r = 7; r >= 0; --r) {
    for (File f = 0; f <= 7; ++f) {
      s += (b & SingleSquare(GetSquareIndex(f, r))).Any() ? "| X " : "|   ";
    }

    s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
  }
  s += "  a   b   c   d   e   f   g   h\n";

  return s;
}
};  // namespace SimpleChessEngine
