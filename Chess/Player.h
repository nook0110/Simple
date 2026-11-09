#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

#include "EnumArray.h"

namespace SimpleChessEngine
{
/**
 * \brief Enum class that represents a player color.
 */
enum class Player : std::uint8_t
{
  kWhite,  //!< White player.
  kBlack   //!< Black player.
};

[[nodiscard]] constexpr Player Flip(const Player player)
{
  return player == Player::kWhite ? Player::kBlack : Player::kWhite;
}

template <>
inline constexpr std::size_t kEnumSize<Player> = 2;

inline constexpr std::array kPlayers{Player::kWhite, Player::kBlack};

template <class Value>
using ByPlayer = EnumArray<Player, Value>;
}  // namespace SimpleChessEngine
