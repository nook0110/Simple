#pragma once

namespace SimpleChessEngine {
/**
 * \brief Enum class that represents a player color.
 */
enum class Player {
  kWhite,  //!< White player.
  kBlack   //!< Black player.
};

[[nodiscard]] inline Player Flip(const Player player) {
  return player == Player::kWhite ? Player::kBlack : Player::kWhite;
}
}  // namespace SimpleChessEngine