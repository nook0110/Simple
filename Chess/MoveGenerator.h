#pragma once

#include <cstdint>
#include <vector>

#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine {
/**
 * \brief Class that all possible moves for a given position.
 *
 * \author nook0110
 */
class MoveGenerator {
 public:
  enum class Type : std::uint8_t {
    kAll = 0,
    kQuiescence = 1,
    kCaptures = 4,
    kQuiets = 8
  };

  static constexpr size_t kMaxMovesPerPosition = 256;
  MoveGenerator() = default;
  ~MoveGenerator();

  using Moves = std::vector<Move>;

  /**
   * \brief Generates all possible moves for a given position.
   *
   * \param position The position.
   *
   * \return All possible moves for the given position.
   */
  template <Type type>
  [[nodiscard]] Moves GenerateMoves(Position& position) const;

  template <Type type>
  [[nodiscard]] Move* GeneratePseudoLegal(Position& position,
                                          Move* moves) const;

  [[nodiscard]] bool HasLegalMove(Position& position) const;

 private:
  /**
   * \brief Generates all possible moves for a given square.
   *
   * \param position The position.
   * \param moves Container where to add possible moves.
   * \param target Target squares.
   *
   * \return All possible moves for the given square.
   */
  template <Piece piece>
  void GenerateMovesForPiece(Move*& moves, Position& position,
                             Bitboard target) const;

  /**
   * \brief Generates all possible moves for a given square with given piece.
   *
   * \tparam piece The piece.
   * \param moves Container where to add moves.
   * \param position The position.
   * \param from The square.
   * \param target Target squares.
   *
   * \return All possible moves for the given square and piece.
   */
  template <Piece piece>
  void GenerateMovesFromSquare(Move*& moves, Position& position, BitIndex from,
                               Bitboard target) const;

  static void GenerateCastling(Move*& moves, const Position& position);
};

}  // namespace SimpleChessEngine
