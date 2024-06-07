#include "MoveGenerator.h"

#include <cassert>

namespace SimpleChessEngine
{
[[nodiscard]] bool MoveGenerator::IsMoveLegal(Position& position,
                                              const Move& move)
{
  const auto us = position.GetSideToMove();

  if (std::holds_alternative<EnCroissant>(move))
  {
    const auto irreversible_data = position.GetIrreversibleData();
    position.DoMove(move);
    const auto valid = !position.IsUnderCheck(us);
    position.UndoMove(move, irreversible_data);
    return valid;
  }

  BitIndex from{};
  BitIndex to{};
  std::visit(
      [&from, &to]<typename MoveType>(const MoveType& unwrapped_move)
      {
        if constexpr (std::same_as<std::remove_cvref_t<MoveType>,
                                   DefaultMove> ||
                      std::same_as<std::remove_cvref_t<MoveType>, PawnPush> ||
                      std::same_as<std::remove_cvref_t<MoveType>, DoublePush> ||
                      std::same_as<std::remove_cvref_t<MoveType>, Promotion>)
        {
          from = unwrapped_move.from;
          to = unwrapped_move.to;
          return;
        }
        assert(false);
      },
      move);
  return !position.GetIrreversibleData().blockers[static_cast<size_t>(us)].Test(
             from) ||
         Ray(position.GetKingSquare(us), from).Test(to);
}

void MoveGenerator::GenerateCastling(Moves& moves, const Position& position)
{
  if (position.IsUnderCheck())
  {
    return;
  }

  const auto side_to_move = position.GetSideToMove();

  const auto king_square = position.GetKingSquare(side_to_move);

  for (const auto castling_side :
       {Castling::CastlingSide::k00, Castling::CastlingSide::k000})
  {
    if (position.CanCastle(castling_side))
    {
      const auto rook_square =
          position.GetCastlingRookSquare(side_to_move, castling_side);
      moves.emplace_back(Castling{castling_side, king_square, rook_square});
    }
  }
}
}  // namespace SimpleChessEngine