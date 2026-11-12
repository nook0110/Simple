#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>

#include "Move.h"

namespace SimpleChessEngine {

class Position;
class MoveGenerator;
template <class Tag> class MaybeMove;
struct PseudoLegalTag {};
struct LegalTag {};

template <class Tag>
class TypedMove {
 public:
  TypedMove() = default;

  template <class OtherTag>
    requires (std::is_same_v<Tag, PseudoLegalTag> &&
              std::is_same_v<OtherTag, LegalTag>)
  [[gnu::always_inline]] constexpr TypedMove(TypedMove<OtherTag> move) : move_(move) {}

  [[gnu::always_inline]] constexpr operator const Move&() const { return move_; }
  [[gnu::always_inline]] constexpr BitIndex From() const { return move_.From(); }
  [[gnu::always_inline]] constexpr BitIndex To() const { return move_.To(); }
  [[gnu::always_inline]] constexpr MoveType Type() const { return move_.Type(); }
  [[gnu::always_inline]] constexpr Piece PromotionPiece() const { return move_.PromotionPiece(); }
  [[gnu::always_inline]] constexpr bool IsValid() const { return move_.IsValid(); }
  [[gnu::always_inline]] constexpr bool IsPromotion() const { return move_.IsPromotion(); }
  [[gnu::always_inline]] constexpr bool IsEnPassant() const { return move_.IsEnPassant(); }
  [[gnu::always_inline]] constexpr bool IsCastling() const { return move_.IsCastling(); }
  [[gnu::always_inline]] constexpr bool IsNormal() const { return move_.IsNormal(); }
  [[gnu::always_inline]] constexpr std::uint16_t Raw() const { return move_.Raw(); }
  [[gnu::always_inline]] constexpr explicit operator bool() const { return static_cast<bool>(move_); }

  template <class PositionType>
  [[gnu::always_inline]] constexpr bool IsQuiet(const PositionType& position) const {
    return move_.IsQuiet(position);
  }

  [[gnu::always_inline]] constexpr bool operator==(const TypedMove&) const = default;
  [[gnu::always_inline]] constexpr bool operator==(const Move& move) const { return move_ == move; }

  template <class OtherTag>
  [[gnu::always_inline]] constexpr bool operator==(const TypedMove<OtherTag>& move) const {
    return move_ == static_cast<Move>(move);
  }

 private:
  friend class MaybeMove<Tag>;
  friend class Position;
  friend class MoveGenerator;

  [[gnu::always_inline]] constexpr explicit TypedMove(Move move) : move_(move) {}

  Move move_;
};

using PseudoLegalMove = TypedMove<PseudoLegalTag>;
using LegalMove = TypedMove<LegalTag>;

static_assert(sizeof(PseudoLegalMove) == sizeof(Move));
static_assert(sizeof(LegalMove) == sizeof(Move));
static_assert(alignof(PseudoLegalMove) == alignof(Move));
static_assert(alignof(LegalMove) == alignof(Move));
static_assert(std::is_trivially_copyable_v<PseudoLegalMove>);
static_assert(std::is_trivially_copyable_v<LegalMove>);
static_assert(std::is_trivially_default_constructible_v<PseudoLegalMove>);
static_assert(std::is_trivially_default_constructible_v<LegalMove>);
static_assert(std::is_standard_layout_v<PseudoLegalMove>);
static_assert(std::is_standard_layout_v<LegalMove>);
static_assert(std::is_convertible_v<LegalMove, PseudoLegalMove>);
static_assert(!std::is_constructible_v<LegalMove, PseudoLegalMove>);
static_assert(!std::is_constructible_v<PseudoLegalMove, Move>);
static_assert(!std::is_constructible_v<LegalMove, Move>);

template <class Tag>
class MaybeMove {
 public:
  MaybeMove() = default;
  [[gnu::always_inline]] constexpr MaybeMove(TypedMove<Tag> move) : move_(move) {}
  [[gnu::always_inline]] constexpr operator const Move&() const { return move_; }
  [[gnu::always_inline]] constexpr explicit operator bool() const { return static_cast<const Move&>(move_).IsValid(); }
  template <class PositionType>
  [[gnu::always_inline]] constexpr bool IsQuiet(const PositionType& position) const {
    return static_cast<const Move&>(move_).IsQuiet(position);
  }
  [[gnu::always_inline]] constexpr bool IsValid() const { return static_cast<const Move&>(move_).IsValid(); }
  [[gnu::always_inline]] constexpr const TypedMove<Tag>& operator*() const {
    assert(IsValid());
    return move_;
  }
  static constexpr MaybeMove None() { return MaybeMove(Move::None()); }
  [[gnu::always_inline]] constexpr bool operator==(const MaybeMove&) const = default;
  template <class OtherTag>
  [[gnu::always_inline]] constexpr bool operator==(const TypedMove<OtherTag>& move) const {
    return move_ == static_cast<Move>(move);
  }
  [[gnu::always_inline]] constexpr bool operator==(const Move& move) const { return move_ == move; }

 private:
  [[gnu::always_inline]] constexpr explicit MaybeMove(Move move) : move_(move) {}
  TypedMove<Tag> move_;
};

using MaybePseudoLegalMove = MaybeMove<PseudoLegalTag>;
using MaybeLegalMove = MaybeMove<LegalTag>;

static_assert(sizeof(MaybePseudoLegalMove) == sizeof(Move));
static_assert(sizeof(MaybeLegalMove) == sizeof(Move));
static_assert(alignof(MaybePseudoLegalMove) == alignof(Move));
static_assert(alignof(MaybeLegalMove) == alignof(Move));
static_assert(std::is_trivially_copyable_v<MaybePseudoLegalMove>);
static_assert(std::is_trivially_copyable_v<MaybeLegalMove>);
static_assert(std::is_trivially_default_constructible_v<MaybePseudoLegalMove>);
static_assert(std::is_trivially_default_constructible_v<MaybeLegalMove>);
static_assert(std::is_standard_layout_v<MaybePseudoLegalMove>);
static_assert(std::is_standard_layout_v<MaybeLegalMove>);

}
