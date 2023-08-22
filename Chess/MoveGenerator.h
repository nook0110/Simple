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
  [[nodiscard]] Moves operator()(Position& position) const;

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
  template <Piece piece>
  [[nodiscard]] Moves GenerateMoves(Position& position, BitIndex from) const;
};

template <>
MoveGenerator::Moves MoveGenerator::GenerateMoves<Piece::kPawn>(
    Position& position, BitIndex from) const;
}  // namespace SimpleChessEngine
