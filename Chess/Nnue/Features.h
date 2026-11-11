#pragma once

#include <array>
#include <cstdint>

#include "../BitBoard.h"
#include "../Piece.h"
#include "../Player.h"
#include "Accumulator.h"
#include "Common.h"

namespace SimpleChessEngine::Nnue::Features {
constexpr IndexType kPieceSquareCount = 11 * 64;
constexpr IndexType kDimensions = 64 * kPieceSquareCount / 2;
constexpr std::uint32_t kHashValue = 0x7f234cb8u;
constexpr IndexType kMaxActiveDimensions = 32;

using IndexList = ValueList<IndexType, kMaxActiveDimensions>;
using ChangedIndexList = ValueList<IndexType, 3>;

consteval std::array<std::array<IndexType, 64>, 2> MakeKingBuckets() {
  std::array<std::array<IndexType, 64>, 2> buckets{};
  for (int square = 0; square < 64; ++square) {
    const int file = square % 8;
    const int rank = square / 8;
    const int mirrored_file = file < 4 ? file : 7 - file;
    buckets[0][square] = static_cast<IndexType>(
        ((7 - rank) * 4 + mirrored_file) * kPieceSquareCount);
    buckets[1][square] =
        static_cast<IndexType>((rank * 4 + mirrored_file) * kPieceSquareCount);
  }
  return buckets;
}

consteval std::array<std::array<int, 64>, 2> MakeOrientation() {
  std::array<std::array<int, 64>, 2> orientation{};
  for (int square = 0; square < 64; ++square) {
    const bool queen_side = square % 8 < 4;
    orientation[0][square] = queen_side ? 7 : 0;
    orientation[1][square] = queen_side ? 63 : 56;
  }
  return orientation;
}

inline constexpr auto kKingBuckets = MakeKingBuckets();
inline constexpr auto kOrientation = MakeOrientation();

template <Player kPerspective>
[[nodiscard]] constexpr IndexType MakeIndex(const BitIndex square,
                                            const Piece piece,
                                            const Player color,
                                            const BitIndex king_square) {
  constexpr auto perspective = static_cast<std::size_t>(kPerspective);
  const IndexType piece_index = piece == Piece::kKing
                                    ? 10 * 64
                                    : (2 * (static_cast<IndexType>(piece) - 1) +
                                       (color == kPerspective ? 0 : 1)) *
                                          64;
  return static_cast<IndexType>(square ^
                                kOrientation[perspective][king_square]) +
         piece_index + kKingBuckets[perspective][king_square];
}

template <Player kPerspective, bool kForward>
void AppendChangedIndices(const BitIndex king_square, const DirtyPiece& dirty,
                          ChangedIndexList& removed, ChangedIndexList& added) {
  for (int i = 0; i < dirty.count; ++i) {
    if (dirty.from[i] != kNoSquare) {
      const IndexType index = MakeIndex<kPerspective>(
          dirty.from[i], dirty.piece[i], dirty.color[i], king_square);
      if constexpr (kForward) {
        removed.push_back(index);
      } else {
        added.push_back(index);
      }
    }
    if (dirty.to[i] != kNoSquare) {
      const IndexType index = MakeIndex<kPerspective>(
          dirty.to[i], dirty.piece[i], dirty.color[i], king_square);
      if constexpr (kForward) {
        added.push_back(index);
      } else {
        removed.push_back(index);
      }
    }
  }
}

[[nodiscard]] inline bool RequiresRefresh(const DirtyPiece& dirty,
                                          const Player perspective) {
  return dirty.count > 0 && dirty.piece[0] == Piece::kKing &&
         dirty.color[0] == perspective;
}
}  // namespace SimpleChessEngine::Nnue::Features
