#pragma once
#include <array>
#include <bit>
#include <cassert>
#include <utility>
#include <variant>
#include <vector>

#include "Hasher.h"
#include "Move.h"
#include "Piece.h"
#include "Position.h"
#include "Utility.h"
namespace SimpleChessEngine {
enum class Bound : std::uint8_t {
  kLower = 1,
  kUpper = 2,
  kExact = kLower | kUpper
};

inline std::uint8_t operator&(const Bound lhs, const Bound rhs) {
  return static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs);
}

enum class PromotionPiece : uint8_t {
  kKnight,  //!< Knight.
  kBishop,  //!< Bishop.
  kRook,    //!< Rook.
  kQueen,   //!< Queen.
};

enum class MoveType : uint8_t { kNone, kEnCroissant, kPromotion, kCastling };

using TTBitIndex = std::uint8_t;

#pragma pack(push, 1)
struct TTMove {
  TTMove(BitIndex from = {}, BitIndex to = {}, MoveType type = {},
         PromotionPiece promotion = {})
      : from(from), to(to), type(type), promotion(promotion) {
    assert(IsOk(from));
    assert(IsOk(to));
    assert(IsOk(this->from));
    assert(IsOk(this->to));
    assert(from == this->from);
    assert(to == this->to);
  }
  TTBitIndex from : 6;
  TTBitIndex to : 6;
  MoveType type : 2;
  PromotionPiece promotion : 2;
};

struct Node {
  Hash true_hash{};
  TTMove move{};
  Eval score{};
  Depth depth : 6 {};
  Bound bound : 2 {};
  Age age{};
};
#pragma pack(pop)

inline Piece Convert(PromotionPiece piece) {
  switch (piece) {
    case PromotionPiece::kKnight:
      return Piece::kKnight;
    case PromotionPiece::kBishop:
      return Piece::kBishop;
    case PromotionPiece::kRook:
      return Piece::kRook;
    case PromotionPiece::kQueen:
      return Piece::kQueen;
  }
  assert(false);
  std::unreachable();
}

inline PromotionPiece Convert(Piece piece) {
  switch (piece) {
    case Piece::kNone:
    case Piece::kPawn:
    case Piece::kKing:
      assert(false);
      std::unreachable();
    case Piece::kKnight:
      return PromotionPiece::kKnight;
    case Piece::kBishop:
      return PromotionPiece::kBishop;
    case Piece::kRook:
      return PromotionPiece::kRook;
    case Piece::kQueen:
      return PromotionPiece::kQueen;
  }
  assert(false);
  std::unreachable();
}

inline Move ConvertMove(TTMove move, const Position& position) {
  auto [from, to, type, promotion] = move;
  switch (type) {
    case MoveType::kNone:
      if (position.GetPieceAt(from) == Piece::kPawn) {
        if (!IsPawnPush(from, to)) {
          return DefaultMove{from, to, position.GetPieceAt(to)};
        }
        if (IsDoublePush(from, to)) {
          return DoublePush{from, to};
        }
        return PawnPush{from, to};
      }
      return DefaultMove{from, to, position.GetPieceAt(to)};
    case MoveType::kEnCroissant:
      return EnCroissant{from, to};
    case MoveType::kPromotion:
      return Promotion{{from, to, position.GetPieceAt(to)}, Convert(promotion)};
    case MoveType::kCastling:
      return Castling{to < from ? Castling::CastlingSide::k000
                                : Castling::CastlingSide::k00,
                      from, to};
  }
}

inline TTMove ConvertMove(const DefaultMove& move) {
  return TTMove{move.from, move.to};
}
inline TTMove ConvertMove(const PawnPush& move) {
  return TTMove{move.from, move.to};
}
inline TTMove ConvertMove(const DoublePush& move) {
  return TTMove{move.from, move.to};
}
inline TTMove ConvertMove(const EnCroissant& move) {
  return TTMove{move.from, move.to, MoveType::kEnCroissant};
}
inline TTMove ConvertMove(const Promotion& move) {
  return TTMove{move.from, move.to, MoveType::kPromotion,
                Convert(move.promoted_to)};
}
inline TTMove ConvertMove(const Castling& move) {
  return TTMove{move.king_from, move.rook_from, MoveType::kCastling};
}
inline TTMove ConvertMove(const Move& move) {
  return std::visit(
      [](const auto& unwrapped_move) { return ConvertMove(unwrapped_move); },
      move);
}

template <size_t TableSize>
  requires(std::has_single_bit(TableSize))
class TranspositionTable {
 public:
  [[nodiscard]] bool Contains(const Position& position) const {
    return position.GetHash() == GetNode(position).true_hash;
  }

  void SetEntry(const Position& position, const Move& move, const Eval score,
                const Depth depth, const Bound bound, const Age age) {
    Node inserting_node = {
        position.GetHash(), ConvertMove(move), score, depth, bound, age};
    auto& entry_node = GetNode(position);
    if (bound == Bound::kExact ||
        !(entry_node.bound == Bound::kExact && entry_node.age == age)) {
      entry_node = inserting_node;
    }
  }

  Node& GetNode(const Position& position) {
    return table_[position.GetHash() % TableSize];
  }

  const Node& GetNode(const Position& position) const {
    return table_[position.GetHash() % TableSize];
  }

  std::vector<Node> table_ = std::vector<Node>(TableSize);  //!< The table.
};
}  // namespace SimpleChessEngine
