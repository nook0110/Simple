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
  Moves operator()(Position& position);

 private:
  /**
   * \brief Generates all possible moves for a given square.
   *
   * \param position The position.
   * \param from The square.
   *
   * \return All possible moves for the given square.
   */
  [[nodiscard]] Moves GenerateMovesForPiece(Position& position,
                                            const BitIndex from);

  /**
   * \brief Generates all possible moves for a given square with given piece.
   *
   * \tparam piece The piece.
   * \param position The position.
   * \param from The square.
   *
   * \return All possible moves for the given square and piece.
   */
  template <Piece piece>
  [[nodiscard]] Moves GenerateMoves(Position& position, const BitIndex from);
};
}  // namespace SimpleChessEngine
