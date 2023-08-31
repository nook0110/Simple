#include "UciCommunicator.h"

int main()
{
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  SimpleChessEngine::UciChessEngine uci;
  uci.Start();
}