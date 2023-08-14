#pragma once

namespace SimpleChessEngine
{
    constexpr size_t kBoardArea = 64;
    constexpr int    kLineSize = 8;
    constexpr size_t kColors = 2;
    constexpr size_t kPieceTypes = 7; // For Pawn, Knight, Bishop, Rook, Queen, King and Empty Square
    constexpr Bitboard<> kBorderBB = 0xff818181818181ff;

    [[nodiscard]] inline constexpr std::pair<int, int> GetCoordinates(const BitIndex square)
    {
        return std::make_pair(square % kLineSize, square / kLineSize);
    }

    [[nodiscard]] inline int ManhattanDistance(const BitIndex first, const BitIndex second)
    {
        const auto [xfirst, yfirst] = GetCoordinates(first);
        const auto [xsecond, ysecond] = GetCoordinates(second);
        return std::abs(xfirst - xsecond) + std::abs(yfirst - ysecond);
    }

    [[nodiscard]] inline int KingDistance(const BitIndex first, const BitIndex second)
    {
        const auto [xfirst, yfirst] = GetCoordinates(first);
        const auto [xsecond, ysecond] = GetCoordinates(second);
        return std::max(std::abs(xfirst - xsecond), std::abs(yfirst - ysecond));
    }

    /**
 * \brief Enum class that represents a player color.
 */
    enum class Player
    {
        kWhite,  //!< White player.
        kBlack   //!< Black player.
    };

    [[nodiscard]] inline Player Flip(const Player player)
    {
        return player == Player::kWhite ? Player::kBlack : Player::kWhite;
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

    [[nodiscard]] inline bool IsOk(const BitIndex square)
    {
        return square >= 0 && square < kBoardArea;
    }

    [[nodiscard]] inline Bitboard<> GetBitboardOfSquare(const BitIndex square)
    {
        return Bitboard<>(1ull << square);
    }

    [[nodiscard]] inline bool IsValidShift(const BitIndex square, const Compass shift)
    {
        BitIndex new_square = square + static_cast<int>(shift);
        return IsOk(new_square) && KingDistance(square, new_square) == 1;
    }
};
