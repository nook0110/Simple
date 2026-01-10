#pragma once
#include <cassert>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

#include "BitBoard.h"
#include "Piece.h"

namespace SimpleChessEngine {

struct NullMove {};

enum class MoveType : std::uint16_t {
  kNormal = 0,
  kCastling = 1 << 14,
  kEnPassant = 2 << 14,
  kPromotion = 3 << 14,
};

class Move {
 private:
  static constexpr std::uint16_t kSquareMask = 0x3F;
  static constexpr std::uint16_t kPromotionMask = 0x3;
  static constexpr std::uint16_t kTypeMask = 0x3;
  static constexpr std::uint8_t kFromShift = 6;
  static constexpr std::uint8_t kPromotionShift = 12;
  static constexpr std::uint8_t kTypeShift = 14;
  static constexpr std::uint16_t kNullValue = 65;
  static constexpr std::uint16_t kNoneValue = 0;

 public:
  Move() = default;
  constexpr explicit Move(std::uint16_t data) : data_(data) {}
  constexpr Move(BitIndex from, BitIndex to)
      : data_((from << kFromShift) + to) {}

  template <MoveType T>
  static constexpr Move Make(BitIndex from, BitIndex to,
                             Piece promotion_piece = Piece::kKnight) {
    return Move(static_cast<std::uint16_t>(T) +
                ((static_cast<std::uint16_t>(promotion_piece) -
                  static_cast<std::uint16_t>(Piece::kKnight))
                 << kPromotionShift) +
                (from << kFromShift) + to);
  }

  constexpr BitIndex From() const {
    assert(IsValid());
    return static_cast<BitIndex>((data_ >> kFromShift) & kSquareMask);
  }

  constexpr BitIndex To() const {
    assert(IsValid());
    return static_cast<BitIndex>(data_ & kSquareMask);
  }

  constexpr MoveType Type() const {
    return static_cast<MoveType>(data_ & (kTypeMask << kTypeShift));
  }

  constexpr Piece PromotionPiece() const {
    return static_cast<Piece>(((data_ >> kPromotionShift) & kPromotionMask) +
                              static_cast<std::uint8_t>(Piece::kKnight));
  }

  constexpr bool IsValid() const {
    return data_ != kNoneValue && data_ != kNullValue;
  }

  constexpr bool IsPromotion() const { return Type() == MoveType::kPromotion; }
  constexpr bool IsEnPassant() const { return Type() == MoveType::kEnPassant; }
  constexpr bool IsCastling() const { return Type() == MoveType::kCastling; }
  constexpr bool IsNormal() const { return Type() == MoveType::kNormal; }

  template <typename Position>
  bool IsQuiet(const Position& position) const {
    if (IsPromotion() || IsEnPassant()) return false;
    if (IsCastling()) return true;
    return position.GetPieceAt(To()) == Piece::kNone;
  }

  static constexpr Move Null() { return Move(kNullValue); }
  static constexpr Move None() { return Move(kNoneValue); }

  constexpr bool operator==(const Move& other) const = default;
  constexpr bool operator!=(const Move& other) const = default;
  constexpr explicit operator bool() const { return data_ != 0; }
  constexpr std::uint16_t Raw() const { return data_; }

 private:
  std::uint16_t data_ = {};
};

enum class CastlingSide : std::uint8_t { k00, k000 };

struct PseudoLegalTag {};
struct LegalTag {};

template <typename T>
concept MoveTag = std::same_as<T, PseudoLegalTag> || std::same_as<T, LegalTag>;

template <typename From, typename To>
concept TagConvertible =
    (std::same_as<From, To>) ||
    (std::same_as<From, LegalTag> && std::same_as<To, PseudoLegalTag>);

template <typename T>
struct RefTypeTraits;

template <>
struct RefTypeTraits<Move&> {
  using ValueType = Move;
  using ReferenceType = Move&;
  using ConstReferenceType = const Move&;
  static constexpr bool is_const = false;
  static constexpr bool is_reference = true;
};

template <>
struct RefTypeTraits<const Move&> {
  using ValueType = Move;
  using ReferenceType = const Move&;
  using ConstReferenceType = const Move&;
  static constexpr bool is_const = true;
  static constexpr bool is_reference = true;
};

template <>
struct RefTypeTraits<Move> {
  using ValueType = Move;
  using ReferenceType = Move&;
  using ConstReferenceType = const Move&;
  static constexpr bool is_const = false;
  static constexpr bool is_reference = false;
};

template <typename Tag, typename RefType = Move>
  requires MoveTag<Tag> &&
           (std::same_as<RefType, Move> || std::same_as<RefType, Move&> ||
            std::same_as<RefType, const Move&>)
class TypedMove {
 private:
  using Traits = RefTypeTraits<RefType>;
  using StorageType = std::conditional_t<
      Traits::is_reference,
      std::conditional_t<Traits::is_const, const Move*, Move*>, Move>;

  StorageType storage_;

  template <bool IsRef = Traits::is_reference>
    requires IsRef
  explicit TypedMove(typename Traits::ReferenceType move) : storage_(&move) {}

  template <bool IsRef = Traits::is_reference>
    requires(!IsRef)
  explicit TypedMove(Move move) : storage_(move) {}

  template <typename T, typename R>
  friend inline TypedMove<T, R> MakeTypedMove(
      typename RefTypeTraits<R>::ReferenceType);

  template <typename T>
  friend inline TypedMove<T, Move> MakeTypedMove(Move);

  template <typename T>
    requires MoveTag<T>
  friend void swap(TypedMove<T, Move>&, TypedMove<T, Move>&) noexcept;

 public:
  template <bool IsRef = Traits::is_reference>
    requires(!IsRef)
  TypedMove() : storage_() {}

  TypedMove(const TypedMove&) = default;
  TypedMove& operator=(const TypedMove&) = default;

  TypedMove(TypedMove&&) noexcept = default;
  TypedMove& operator=(TypedMove&&) noexcept = default;

  // Conversion from TypedMove<Tag, Move> to TypedMove<Tag, Move&>
  template <typename OtherRefType>
    requires(std::same_as<RefType, Move&> && std::same_as<OtherRefType, Move>)
  TypedMove(TypedMove<Tag, OtherRefType>& other) : storage_(&other.get()) {}

  // Conversion from TypedMove<Tag, Move> to TypedMove<Tag, const Move&>
  template <typename OtherRefType>
    requires(std::same_as<RefType, const Move&> &&
             (std::same_as<OtherRefType, Move> ||
              std::same_as<OtherRefType, Move&>))
  TypedMove(const TypedMove<Tag, OtherRefType>& other)
      : storage_(&other.get()) {}

  template <typename OtherTag, typename OtherRefType>
    requires TagConvertible<OtherTag, Tag> &&
             std::same_as<RefType, OtherRefType>
  TypedMove(const TypedMove<OtherTag, OtherRefType>& other) {
    if constexpr (Traits::is_reference) {
      storage_ = &other.get();
    } else {
      storage_ = other.get();
    }
  }

  constexpr typename Traits::ConstReferenceType get() const {
    if constexpr (Traits::is_reference) {
      return *storage_;
    } else {
      return storage_;
    }
  }

  constexpr typename Traits::ReferenceType get()
    requires(!Traits::is_const)
  {
    if constexpr (Traits::is_reference) {
      return *storage_;
    } else {
      return storage_;
    }
  }

  constexpr operator typename Traits::ConstReferenceType() const& {
    return get();
  }

  constexpr operator Move() const&& {
    if constexpr (Traits::is_reference) {
      return *storage_;
    } else {
      return storage_;
    }
  }

  constexpr operator typename Traits::ReferenceType() &
    requires(!Traits::is_const)
  {
    return get();
  }

  // Conversion from reference types to value type
  template <typename R = RefType>
    requires(Traits::is_reference)
  constexpr operator TypedMove<Tag, Move>() const {
    return MakeTypedMove<Tag>(get());
  }

  constexpr BitIndex From() const { return get().From(); }
  constexpr BitIndex To() const { return get().To(); }
  constexpr MoveType Type() const { return get().Type(); }
  constexpr Piece PromotionPiece() const { return get().PromotionPiece(); }
  constexpr bool IsValid() const { return get().IsValid(); }
  constexpr bool IsPromotion() const { return get().IsPromotion(); }
  constexpr bool IsEnPassant() const { return get().IsEnPassant(); }
  constexpr bool IsCastling() const { return get().IsCastling(); }
  constexpr bool IsNormal() const { return get().IsNormal(); }
  constexpr std::uint16_t Raw() const { return get().Raw(); }
  constexpr explicit operator bool() const { return get().operator bool(); }

  template <typename Position>
  bool IsQuiet(const Position& position) const {
    return get().IsQuiet(position);
  }

  constexpr bool operator==(const TypedMove& other) const {
    return get() == other.get();
  }
  constexpr bool operator!=(const TypedMove& other) const {
    return get() != other.get();
  }

  constexpr bool operator==(const Move& other) const { return get() == other; }
  constexpr bool operator!=(const Move& other) const { return get() != other; }
};

template <typename Tag>
  requires MoveTag<Tag>
void swap(TypedMove<Tag, Move>& lhs, TypedMove<Tag, Move>& rhs) noexcept {
  Move temp = lhs.get();
  lhs = TypedMove<Tag, Move>(rhs.get());
  rhs = TypedMove<Tag, Move>(temp);
}

template <typename Tag, typename RefType>
inline TypedMove<Tag, RefType> MakeTypedMove(
    typename RefTypeTraits<RefType>::ReferenceType move) {
  static_assert(MoveTag<Tag>, "Tag must be a valid MoveTag");
  return TypedMove<Tag, RefType>(move);
}

template <typename Tag>
inline TypedMove<Tag, Move> MakeTypedMove(Move move) {
  static_assert(MoveTag<Tag>, "Tag must be a valid MoveTag");
  return TypedMove<Tag, Move>(move);
}

using PseudoLegalMoveRef = TypedMove<PseudoLegalTag, Move&>;
using PseudoLegalMoveConstRef = TypedMove<PseudoLegalTag, const Move&>;
using LegalMoveRef = TypedMove<LegalTag, Move&>;
using LegalMoveConstRef = TypedMove<LegalTag, const Move&>;

using PseudoLegalMove = TypedMove<PseudoLegalTag, Move>;
using LegalMove = TypedMove<LegalTag, Move>;

static_assert(std::is_constructible_v<LegalMoveRef, LegalMove&>,
              "LegalMoveRef must be constructible from LegalMove&");
static_assert(std::is_constructible_v<LegalMoveConstRef, const LegalMove&>,
              "LegalMoveConstRef must be constructible from const LegalMove&");
static_assert(std::is_constructible_v<LegalMoveConstRef, LegalMove&>,
              "LegalMoveConstRef must be constructible from LegalMove&");

static_assert(std::is_constructible_v<PseudoLegalMoveRef, PseudoLegalMove&>,
              "PseudoLegalMoveRef must be constructible from PseudoLegalMove&");
static_assert(
    std::is_constructible_v<PseudoLegalMoveConstRef, const PseudoLegalMove&>,
    "PseudoLegalMoveConstRef must be constructible from const "
    "PseudoLegalMove&");
static_assert(
    std::is_constructible_v<PseudoLegalMoveConstRef, PseudoLegalMove&>,
    "PseudoLegalMoveConstRef must be constructible from PseudoLegalMove&");

template <typename Tag>
  requires MoveTag<Tag>
class MoveList {
 public:
  using ValueType = Move;
  using Container = std::vector<Move>;
  using SizeType = typename Container::size_type;

  using Reference = TypedMove<Tag, Move&>;
  using ConstReference = TypedMove<Tag, const Move&>;

  template <bool IsConst>
  class Iterator {
   private:
    using BaseIterator =
        std::conditional_t<IsConst, typename Container::const_iterator,
                           typename Container::iterator>;

    BaseIterator iter_;

   public:
    struct ArrowProxy {
      std::conditional_t<IsConst, ConstReference, Reference> move;
      auto operator->() { return &move; }
      auto operator->() const { return &move; }
    };

    using iterator_category = std::random_access_iterator_tag;
    using value_type = Move;
    using difference_type = typename BaseIterator::difference_type;
    using pointer = ArrowProxy;
    using reference = std::conditional_t<IsConst, ConstReference, Reference>;

    Iterator() = default;
    explicit Iterator(BaseIterator iter) : iter_(iter) {}

    template <bool WasConst>
      requires(IsConst && !WasConst)
    Iterator(const Iterator<WasConst>& other) : iter_(other.iter_) {}

    reference operator*() const {
      if constexpr (IsConst) {
        return MakeTypedMove<Tag, const Move&>(*iter_);
      } else {
        return MakeTypedMove<Tag, Move&>(*iter_);
      }
    }

    pointer operator->() const {
      if constexpr (IsConst) {
        return ArrowProxy{MakeTypedMove<Tag, const Move&>(*iter_)};
      } else {
        return ArrowProxy{MakeTypedMove<Tag, Move&>(*iter_)};
      }
    }

    Iterator& operator++() {
      ++iter_;
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++iter_;
      return tmp;
    }
    Iterator& operator--() {
      --iter_;
      return *this;
    }
    Iterator operator--(int) {
      Iterator tmp = *this;
      --iter_;
      return tmp;
    }

    Iterator& operator+=(difference_type n) {
      iter_ += n;
      return *this;
    }
    Iterator& operator-=(difference_type n) {
      iter_ -= n;
      return *this;
    }

    Iterator operator+(difference_type n) const { return Iterator(iter_ + n); }
    Iterator operator-(difference_type n) const { return Iterator(iter_ - n); }

    difference_type operator-(const Iterator& other) const {
      return iter_ - other.iter_;
    }

    reference operator[](difference_type n) const {
      if constexpr (IsConst) {
        return MakeTypedMove<Tag, const Move&>(iter_[n]);
      } else {
        return MakeTypedMove<Tag, Move&>(iter_[n]);
      }
    }

    bool operator==(const Iterator& other) const {
      return iter_ == other.iter_;
    }
    bool operator!=(const Iterator& other) const {
      return iter_ != other.iter_;
    }
    bool operator<(const Iterator& other) const { return iter_ < other.iter_; }
    bool operator<=(const Iterator& other) const {
      return iter_ <= other.iter_;
    }
    bool operator>(const Iterator& other) const { return iter_ > other.iter_; }
    bool operator>=(const Iterator& other) const {
      return iter_ >= other.iter_;
    }

    template <bool OtherConst>
    bool operator==(const Iterator<OtherConst>& other) const {
      return iter_ == other.iter_;
    }
    template <bool OtherConst>
    bool operator!=(const Iterator<OtherConst>& other) const {
      return iter_ != other.iter_;
    }

    template <bool>
    friend class Iterator;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  MoveList() = default;
  explicit MoveList(Container&& moves) : moves_(std::move(moves)) {}
  explicit MoveList(const Container& moves) : moves_(moves) {}

  [[nodiscard]] bool empty() const noexcept { return moves_.empty(); }
  [[nodiscard]] SizeType size() const noexcept { return moves_.size(); }
  [[nodiscard]] SizeType capacity() const noexcept { return moves_.capacity(); }
  void reserve(SizeType n) { moves_.reserve(n); }
  void clear() noexcept { moves_.clear(); }

  Reference operator[](SizeType pos) {
    return MakeTypedMove<Tag, Move&>(moves_[pos]);
  }

  ConstReference operator[](SizeType pos) const {
    return MakeTypedMove<Tag, const Move&>(moves_[pos]);
  }

  Reference front() { return MakeTypedMove<Tag, Move&>(moves_.front()); }
  ConstReference front() const {
    return MakeTypedMove<Tag, const Move&>(moves_.front());
  }

  Reference back() { return MakeTypedMove<Tag, Move&>(moves_.back()); }
  ConstReference back() const {
    return MakeTypedMove<Tag, const Move&>(moves_.back());
  }

  iterator begin() noexcept { return iterator(moves_.begin()); }
  const_iterator begin() const noexcept {
    return const_iterator(moves_.begin());
  }
  const_iterator cbegin() const noexcept {
    return const_iterator(moves_.begin());
  }

  iterator end() noexcept { return iterator(moves_.end()); }
  const_iterator end() const noexcept { return const_iterator(moves_.end()); }
  const_iterator cend() const noexcept { return const_iterator(moves_.end()); }

  void push_back(TypedMove<Tag, Move> move) { moves_.push_back(move.get()); }

  template <typename... Args>
  Reference emplace_back(Args&&... args) {
    moves_.emplace_back(std::forward<Args>(args)...);
    return MakeTypedMove<Tag, Move&>(moves_.back());
  }

  void pop_back() { moves_.pop_back(); }

  [[nodiscard]] Container release() noexcept { return std::move(moves_); }

  [[nodiscard]] const Container& data() const noexcept { return moves_; }

 private:
  Container moves_;
};

static_assert(sizeof(Move) == 2, "Move must be exactly 2 bytes");
static_assert(alignof(Move) == 2, "Move should be 2-byte aligned");
static_assert(std::is_trivially_copyable_v<Move>,
              "Move must be trivially copyable");
static_assert(std::is_standard_layout_v<Move>,
              "Move must have standard layout");

static_assert(sizeof(PseudoLegalMove) == sizeof(Move),
              "PseudoLegalMove must have same size as Move");
static_assert(sizeof(LegalMove) == sizeof(Move),
              "LegalMove must have same size as Move");
static_assert(sizeof(PseudoLegalMoveRef) == sizeof(Move*),
              "PseudoLegalMoveRef must be pointer-sized");
static_assert(sizeof(LegalMoveRef) == sizeof(Move*),
              "LegalMoveRef must be pointer-sized");

static_assert(std::is_convertible_v<LegalMove, PseudoLegalMove>,
              "LegalMove must be convertible to PseudoLegalMove");
static_assert(std::is_convertible_v<LegalMove, Move>,
              "LegalMove must be convertible to Move");
static_assert(std::is_convertible_v<PseudoLegalMove, Move>,
              "PseudoLegalMove must be convertible to Move");

static_assert(!std::is_convertible_v<Move, PseudoLegalMove>,
              "Move must not be convertible to PseudoLegalMove");
static_assert(!std::is_convertible_v<PseudoLegalMove, LegalMove>,
              "PseudoLegalMove must not be convertible to LegalMove");
static_assert(!std::is_convertible_v<Move, LegalMove>,
              "Move must not be convertible to LegalMove");

static_assert(std::is_convertible_v<LegalMoveRef, PseudoLegalMoveRef>,
              "LegalMoveRef must be convertible to PseudoLegalMoveRef");
static_assert(!std::is_convertible_v<PseudoLegalMoveRef, LegalMoveRef>,
              "PseudoLegalMoveRef must not be convertible to LegalMoveRef");

class Position;

template <typename To, typename From>
To UnsafeMoveCast(From move) {
  static_assert(
      std::is_same_v<To, PseudoLegalMove> || std::is_same_v<To, LegalMove>,
      "To must be PseudoLegalMove or LegalMove");
  static_assert(
      std::is_same_v<From, Move> || std::is_same_v<From, PseudoLegalMove>,
      "From must be Move or PseudoLegalMove");

  if constexpr (std::is_same_v<To, PseudoLegalMove> &&
                std::is_same_v<From, Move>) {
    return MakeTypedMove<PseudoLegalTag>(move);
  } else if constexpr (std::is_same_v<To, LegalMove> &&
                       std::is_same_v<From, PseudoLegalMove>) {
    return MakeTypedMove<LegalTag>(static_cast<Move>(move));
  } else if constexpr (std::is_same_v<To, LegalMove> &&
                       std::is_same_v<From, Move>) {
    return MakeTypedMove<LegalTag>(move);
  } else {
    return move;
  }
}
}  // namespace SimpleChessEngine

namespace std {
template <typename Tag>
  requires SimpleChessEngine::MoveTag<Tag>
void swap(
    SimpleChessEngine::TypedMove<Tag, SimpleChessEngine::Move&> lhs,
    SimpleChessEngine::TypedMove<Tag, SimpleChessEngine::Move&> rhs) noexcept {
  std::swap(lhs.get(), rhs.get());
}
}  // namespace std
