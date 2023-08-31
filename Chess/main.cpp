#include "UciCommunicator.h"

int main()
{
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  const auto start_pos = SimpleChessEngine::PositionFactory{}();
  SimpleChessEngine::Position pos = start_pos;

  for (const auto moves =
    SimpleChessEngine::MoveGenerator{}.GenerateMoves<false>(pos);
    const auto & move : moves)
  {
    const auto irreversible_data = pos.GetIrreversibleData();
    pos.DoMove(move);
    pos.UndoMove(move, irreversible_data);
  }
  SimpleChessEngine::UciChessEngine uci;
  uci.Start();
}