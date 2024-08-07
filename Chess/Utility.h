#pragma once

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

#include "BitBoard.h"
#include "Piece.h"
#include "Player.h"

namespace SimpleChessEngine {
using Depth = uint8_t;
using Age = uint16_t;
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

constexpr size_t kBoardArea = 64;
constexpr Bitboard kEmptyBoard = Bitboard{};
constexpr int8_t kLineSize = 8;
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

constexpr std::array<std::array<BitIndex, 2>, kColors>
    kKingCastlingDestination = {{{6, 2}, {62, 58}}};
constexpr std::array<std::array<BitIndex, 2>, kColors>
    kRookCastlingDestination = {{{5, 3}, {61, 59}}};

using Rank = int8_t;
using File = int8_t;

using Coordinates = std::pair<File, Rank>;

[[nodiscard]] constexpr Coordinates GetCoordinates(const BitIndex square) {
  return std::make_pair(square % kLineSize, square / kLineSize);
}

[[nodiscard]] constexpr BitIndex GetSquare(const File file, const Rank rank) {
  assert(file >= 0 && file < kLineSize);
  assert(rank >= 0 && rank < kLineSize);
  return rank << 3 | file;
}

[[nodiscard]] inline int ManhattanDistance(const BitIndex first,
                                           const BitIndex second) {
  const auto [x_first, y_first] = GetCoordinates(first);
  const auto [x_second, y_second] = GetCoordinates(second);
  return std::abs(x_first - x_second) + std::abs(y_first - y_second);
}

[[nodiscard]] inline int KingDistance(const BitIndex first,
                                      const BitIndex second) {
  const auto [x_first, y_first] = GetCoordinates(first);
  const auto [x_second, y_second] = GetCoordinates(second);
  return std::max(std::abs(x_first - x_second), std::abs(y_first - y_second));
}

enum class CastlingRights : uint8_t {
  kNone,
  k00 = 1,
  k000 = 2,
  kAll = k00 | k000
};

constexpr std::array kCastlingRightsForSide = {CastlingRights::k00,
                                               CastlingRights::k000};

enum class Compass : int8_t {
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
  return static_cast<Compass>(-static_cast<int8_t>(direction));
}

[[nodiscard]] inline Bitboard Shift(const Bitboard bb,
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

[[nodiscard]] inline bool IsOk(const BitIndex square) {
  return 0 <= square && square < kBoardArea;
}

[[nodiscard]] inline Bitboard GetBitboardOfSquare(const BitIndex square) {
  return Bitboard{1ull << square};
}

[[nodiscard]] inline bool IsAdjacent(const BitIndex sq_first,
                                     const BitIndex sq_second) {
  return KingDistance(sq_first, sq_second) == 1;
}

[[nodiscard]] inline bool IsShiftValid(const BitIndex shifted_square,
                                       const BitIndex square) {
  return IsOk(shifted_square) && IsAdjacent(square, shifted_square);
}

[[nodiscard]] inline std::optional<Bitboard> GetShiftIfValid(
    const BitIndex square, const Compass shift) {
  const BitIndex new_square = Shift(square, shift);
  return IsShiftValid(new_square, square)
             ? std::optional{GetBitboardOfSquare(new_square)}
             : std::nullopt;
}

[[nodiscard]] inline std::optional<Bitboard> DoShiftIfValid(
    BitIndex& square, const Compass shift) {
  const BitIndex copy = square;
  square += static_cast<int>(shift);
  return IsShiftValid(square, copy) ? std::optional{GetBitboardOfSquare(square)}
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

constexpr std::array kCheckers = {Piece::kKnight, Piece::kBishop, Piece::kRook,
                                  Piece::kQueen};

inline std::array<std::array<Bitboard, kBoardArea>, kBoardArea>
    bishop_between{};
inline std::array<std::array<Bitboard, kBoardArea>, kBoardArea> rook_between{};
inline std::array<std::array<Bitboard, kBoardArea>, kBoardArea> bishop_ray{};
inline std::array<std::array<Bitboard, kBoardArea>, kBoardArea> rook_ray{};

inline Bitboard Between(const BitIndex from, const BitIndex to) {
  return bishop_between[from][to] | rook_between[from][to];
}

inline Bitboard Ray(const BitIndex from, const BitIndex to) {
  return bishop_ray[from][to] | rook_ray[from][to];
}

inline std::array<std::array<Bitboard, kBoardArea>, kColors> pawn_attacks{};

inline void InitPawnAttacks() {
  for (auto color : {Player::kWhite, Player::kBlack}) {
    for (BitIndex square = 0; square < kBoardArea; ++square) {
      Bitboard res{};
      for (const auto step :
           kPawnAttackDirections[static_cast<size_t>(color)]) {
        res |= GetShiftIfValid(square, step).value_or(Bitboard{});
      }
      pawn_attacks[static_cast<size_t>(color)][square] = res;
    }
  }
}

[[nodiscard]] inline Bitboard GetPawnAttacks(const BitIndex square,
                                             const Player side) {
  return pawn_attacks[static_cast<size_t>(side)][square];
}

[[nodiscard]] inline std::string DrawBitboard(const Bitboard b) {
  std::string s = "+---+---+---+---+---+---+---+---+\n";

  for (Rank r = 7; r >= 0; --r) {
    for (File f = 0; f <= 7; ++f) {
      s += (b & GetBitboardOfSquare(GetSquare(f, r))).Any() ? "| X " : "|   ";
    }

    s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
  }
  s += "  a   b   c   d   e   f   g   h\n";

  return s;
}
};  // namespace SimpleChessEngine