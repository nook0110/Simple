#pragma once
#include <cstdint>
#include <cassert>
#include <tuple>
#include "BitBoard.h"
#include "Piece.h"

namespace SimpleChessEngine {

struct NullMove {};

enum class MoveType : std::uint16_t {
    kNormal    = 0,
    kCastling  = 1 << 14,
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
    constexpr Move(BitIndex from, BitIndex to) : data_((from << kFromShift) + to) {}

    template<MoveType T>
    static constexpr Move Make(BitIndex from, BitIndex to, Piece promotion_piece = Piece::kKnight) {
        return Move(static_cast<std::uint16_t>(T) + 
                   ((static_cast<std::uint16_t>(promotion_piece) - static_cast<std::uint16_t>(Piece::kKnight)) << kPromotionShift) + 
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
        return static_cast<Piece>(((data_ >> kPromotionShift) & kPromotionMask) + static_cast<std::uint8_t>(Piece::kKnight));
    }

    constexpr bool IsValid() const {
        return data_ != kNoneValue && data_ != kNullValue;
    }

    constexpr bool IsPromotion() const { return Type() == MoveType::kPromotion; }
    constexpr bool IsEnPassant() const { return Type() == MoveType::kEnPassant; }
    constexpr bool IsCastling() const { return Type() == MoveType::kCastling; }
    constexpr bool IsNormal() const { return Type() == MoveType::kNormal; }
    
    template<typename Position>
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

    struct Hash {
        std::size_t operator()(const Move& move) const {
            return move.data_ * 6364136223846793005ULL + 1442695040888963407ULL;
        }
    };

private:
    std::uint16_t data_;
};

enum class CastlingSide : std::uint8_t { k00, k000 };

static_assert(sizeof(Move) == 2, "Move must be exactly 2 bytes");
static_assert(alignof(Move) == 2, "Move should be 2-byte aligned");
static_assert(std::is_trivially_copyable_v<Move>, "Move must be trivially copyable");
static_assert(std::is_standard_layout_v<Move>, "Move must have standard layout");

}  // namespace SimpleChessEngine
