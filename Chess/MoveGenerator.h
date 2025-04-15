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
  enum class Type : uint8_t { kAll = 0, kQuiescence = 1, kAddChecks = 2 };

  static constexpr size_t kMaxMovesPerPosition = 218;
  MoveGenerator() { moves_.reserve(kMaxMovesPerPosition); }
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

 private:
  [[nodiscard]] static bool IsPawnMoveLegal(Position& position,
                                            const Move& move);

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
  void GenerateMovesForPiece(Moves& moves, Position& position,
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
  void GenerateMovesFromSquare(Moves& moves, Position& position, BitIndex from,
                               Bitboard target) const;

  static void GenerateCastling(Moves& moves, const Position& position);

  mutable Moves moves_;
};

}  // namespace SimpleChessEngine
