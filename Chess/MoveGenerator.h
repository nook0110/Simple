#pragma once
#include <vector>

#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine
{
/**
 * \brief Class that all possible moves for a given position.
 *
 * \author nook0110
 */
class MoveGenerator
{
 public:
  using Moves = std::vector<Move>;

  /**
   * \brief Generates all possible moves for a given position.
   *
   * \param position The position.
   *
   * \return All possible moves for the given position.
   */
  template <bool only_attacks = false>
  [[nodiscard]] Moves GenerateMoves(Position& position) const;

 private:
  [[nodiscard]] static bool IsMoveValid(Position& position, const Move& move)
  {
    position.DoMove(move);
    const auto valid = !position.IsUnderCheck(Flip(position.GetSideToMove()));
    position.UndoMove(move);

    return valid;
  }

  /**
   * \brief Generates all possible moves for a given square.
   *
   * \param position The position.
   * \param from The square.
   *
   * \return All possible moves for the given square.
   */
  template <bool only_attacks = false>
  [[nodiscard]] Moves GenerateMovesForPiece(Position& position,
                                            BitIndex from) const;

  /**
   * \brief Generates all possible moves for a given square with given piece.
   *
   * \tparam piece The piece.
   * \param position The position.
   * \param from The square.
   *
   * \return All possible moves for the given square and piece.
   */
  template <Piece piece, bool only_attacks = false>
  [[nodiscard]] Moves GenerateMovesFromSquare(Position& position,
                                              BitIndex from) const;

  [[nodiscard]] Moves GenerateCastling(Position& position) const;

  [[nodiscard]] static Moves GenerateAttacksForPawn(Position& position,
                                                    BitIndex from);

  [[nodiscard]] static Moves GenerateMovesForPawn(Position& position,
                                                  BitIndex from);
  [[nodiscard]] static Moves ApplyPromotions(Moves moves,
                                             const Position& position,
                                             BitIndex from);
};
}  // namespace SimpleChessEngine
