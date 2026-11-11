#pragma once

#include <array>

#include "BitBoard.h"
#include "Piece.h"
#include "Utility.h"

namespace SimpleChessEngine {
template <Piece piece>
  requires(piece == Piece::kKing || piece == Piece::kKnight)
consteval auto MakeLeaperAttacks() {
  std::array<Bitboard, kBoardArea> attacks{};
  for (BitIndex square = 0; square < static_cast<BitIndex>(kBoardArea);
       ++square) {
    const auto [file, rank] = GetCoordinates(square);
    for (int df = -2; df <= 2; ++df) {
      for (int dr = -2; dr <= 2; ++dr) {
        const int file_distance = df < 0 ? -df : df;
        const int rank_distance = dr < 0 ? -dr : dr;
        const bool attacks_square =
            piece == Piece::kKing ? std::max(file_distance, rank_distance) == 1
                                  : file_distance * rank_distance == 2;
        if (attacks_square && file + df >= 0 && file + df < kLineSize &&
            rank + dr >= 0 && rank + dr < kLineSize) {
          attacks[square].Set(GetSquareIndex(file + df, rank + dr));
        }
      }
    }
  }
  return attacks;
}

inline constexpr auto kKingAttacks = MakeLeaperAttacks<Piece::kKing>();
inline constexpr auto kKnightAttacks = MakeLeaperAttacks<Piece::kKnight>();

template <Piece sliding_piece>
constexpr size_t GetMagicShift() {
  static_assert(IsWeakSlidingPiece(sliding_piece));
  if constexpr (sliding_piece == Piece::kBishop) return 64 - 9;
  if constexpr (sliding_piece == Piece::kRook) return 64 - 12;
}

/**
 * \brief
 *
 */
struct Magic {
  Bitboard mask;
  Bitboard magic;
  size_t base_offset;
  template <Piece sliding_piece>
  constexpr size_t GetAddress(const Bitboard occupied) const {
    static_assert(IsWeakSlidingPiece(sliding_piece));
    return base_offset + static_cast<size_t>((mask & occupied) * magic >>
                                             GetMagicShift<sliding_piece>());
  }
};

inline constexpr size_t kSliderTableSize = 88772;
extern const std::array<Magic, kBoardArea> kBishopMagics;
extern const std::array<Magic, kBoardArea> kRookMagics;
extern const std::array<Bitboard, kSliderTableSize> kSliderAttacks;

template <Piece piece>
  requires(IsSlidingPiece(piece) || piece == Piece::kKnight ||
           piece == Piece::kKing)
class AttackTable {
 public:
  AttackTable() = delete;
  [[nodiscard]] static Bitboard GetAttackMap(BitIndex square,
                                             Bitboard occupied);
};

template <Piece piece>
  requires(IsSlidingPiece(piece) || piece == Piece::kKnight ||
           piece == Piece::kKing)
Bitboard AttackTable<piece>::GetAttackMap(const BitIndex square,
                                          const Bitboard occupied) {
  if constexpr (piece == Piece::kKnight) {
    return kKnightAttacks[square];
  }
  if constexpr (piece == Piece::kKing) {
    return kKingAttacks[square];
  }
  if constexpr (piece == Piece::kQueen) {
    return AttackTable<Piece::kBishop>::GetAttackMap(square, occupied) |
           AttackTable<Piece::kRook>::GetAttackMap(square, occupied);
  }
  if constexpr (piece == Piece::kBishop) {
    return kSliderAttacks[kBishopMagics[square].GetAddress<piece>(occupied)];
  }
  if constexpr (piece == Piece::kRook) {
    return kSliderAttacks[kRookMagics[square].GetAddress<piece>(occupied)];
  }
}

extern template class AttackTable<Piece::kKnight>;
extern template class AttackTable<Piece::kBishop>;
extern template class AttackTable<Piece::kRook>;
extern template class AttackTable<Piece::kQueen>;
extern template class AttackTable<Piece::kKing>;

template <Piece sliding_piece>
  requires(IsWeakSlidingPiece(sliding_piece))
[[nodiscard]] Bitboard GenerateAttackMask(
    const BitIndex square, const Bitboard occupancy = kEmptyBoard);
}  // namespace SimpleChessEngine
