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
  enum class Type
  {
    kDefault,
    kQuiescence
  };

  MoveGenerator() { moves_.reserve(256); }

  using Moves = std::vector<Move>;

  /**
   * \brief Generates all possible moves for a given position.
   *
   * \param position The position.
   *
   * \return All possible moves for the given position.
   */
  template <Type type>
  [[nodiscard]] Moves GenerateMoves(Position& position);

 private:
  [[nodiscard]] static bool IsMoveValid(Position& position, const Move& move)
  {
    const auto us = position.GetSideToMove();
    const auto irreversible_data = position.GetIrreversibleData();

    if (const auto default_move = std::get_if<DefaultMove>(&move))
    {
      if (position.GetPiece(default_move->from) == Piece::kKing)
      {
        return !position.IsUnderAttack(default_move->to, us, GetBitboardOfSquare(default_move->from));
      }
      assert(position.GetPiece(default_move->from) == Piece::kPawn);
      return !irreversible_data.blockers[static_cast<size_t>(us)].Test(default_move->from) ||
        Ray(position.GetKingSquare(us), default_move->from).Test(default_move->to);
    }
    if (std::holds_alternative<EnCroissant>(move))
    {
      position.DoMove(move);
      const auto valid = !position.IsUnderCheck(us);
      position.UndoMove(move, irreversible_data);
      return valid;
    }
    BitIndex from{};
    BitIndex to{};
    std::visit([&from, &to](const auto& unwrapped_move)
      {
        if constexpr (std::same_as<std::remove_cvref_t<decltype(unwrapped_move)>, PawnPush> ||
                      std::same_as<std::remove_cvref_t<decltype(unwrapped_move)>, DoublePush> ||
                      std::same_as<std::remove_cvref_t<decltype(unwrapped_move)>, Promotion>)
        {
          from = unwrapped_move.from;
          to = unwrapped_move.to;
          return;
        }
        assert(false);
      }, move);
    return !irreversible_data.blockers[static_cast<size_t>(us)].Test(from) ||
            Ray(position.GetKingSquare(us), from).Test(to);
  }

  /**
   * \brief Generates all possible moves for a given square.
   *
   * \param position The position.
   * \param from The square.
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
   * \param position The position.
   * \param from The square.
   *
   * \return All possible moves for the given square and piece.
   */
  template <Piece piece>
  void GenerateMovesFromSquare(Moves& moves, Position& position, BitIndex from,
                               Bitboard target) const;

  static void GenerateCastling(Moves& moves, const Position& position);

  Moves moves_;
};
}  // namespace SimpleChessEngine
