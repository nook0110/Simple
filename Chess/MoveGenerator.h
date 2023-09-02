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
  MoveGenerator() { moves_.reserve(256); }

  using Moves = std::vector<Move>;

  /**
   * \brief Generates all possible moves for a given position.
   *
   * \param position The position.
   *
   * \return All possible moves for the given position.
   */
  template <bool only_attacks>
  [[nodiscard]] Moves GenerateMoves(Position& position);

 private:
  [[nodiscard]] static bool IsMoveValid(Position& position, const Move& move)
  {
    const auto irreversible_data = position.GetIrreversibleData();

    position.DoMove(move);
    const auto valid = !position.IsUnderCheck(Flip(position.GetSideToMove()));
    position.UndoMove(move, irreversible_data);

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
  template <bool only_attacks>
  void GenerateMovesForPiece(Moves& moves, Position& position,
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
  template <Piece piece, bool only_attacks>
  void GenerateMovesFromSquare(Moves& moves, Position& position,
                               BitIndex from) const;

  void GenerateCastling(Moves& moves, const Position& position) const;

  static void GenerateAttacksForPawn(Moves& moves, Position& position,
                                     BitIndex from);

  static void GenerateMovesForPawn(Moves& moves, Position& position,
                                   BitIndex from);
  static void ApplyPromotions(size_t begin, size_t end, Moves& moves,
                              const Position& position, BitIndex from);

  Moves moves_;
};
}  // namespace SimpleChessEngine
