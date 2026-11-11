#pragma once

#include <memory>
#include <utility>

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
  assert(IsWeakSlidingPiece(sliding_piece));
  if constexpr (sliding_piece == Piece::kBishop) return 64 - 9;
  if constexpr (sliding_piece == Piece::kRook) return 64 - 12;
  assert(false);
  std::unreachable();
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
  size_t GetAddress(const Bitboard occupied) const {
    assert(IsWeakSlidingPiece(sliding_piece));
    return base_offset + static_cast<size_t>((mask & occupied) * magic >>
                                             GetMagicShift<sliding_piece>());
  }
};

template <Piece sliding_piece>
class AttackTable {
 public:
  static Bitboard GetAttackMap(BitIndex square, Bitboard occupied);

  AttackTable();

 private:
  static size_t GetAttackTableAddress(BitIndex square,
                                      Bitboard occupied = kEmptyBoard);

  Bitboard *table_ = nullptr;
  std::array<Magic, kBoardArea> magic_ = {};

  static const std::unique_ptr<AttackTable> self_;
};

template <Piece piece>
size_t AttackTable<piece>::GetAttackTableAddress(const BitIndex square,
                                                 const Bitboard occupied) {
  assert(IsWeakSlidingPiece(piece));
  return self_->magic_[square].template GetAddress<piece>(occupied);
}

template <Piece piece>
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
  if constexpr (IsWeakSlidingPiece(piece)) {
    return self_->table_[GetAttackTableAddress(square, occupied)];
  }
  assert(false);
  std::unreachable();
}

extern template class AttackTable<Piece::kKnight>;
extern template class AttackTable<Piece::kBishop>;
extern template class AttackTable<Piece::kRook>;
extern template class AttackTable<Piece::kQueen>;
extern template class AttackTable<Piece::kKing>;

template <Piece sliding_piece>
[[nodiscard]] Bitboard GenerateAttackMask(
    const BitIndex square, const Bitboard occupancy = kEmptyBoard);
}  // namespace SimpleChessEngine
