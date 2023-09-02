#include "Perft.h"
#include "UciCommunicator.h"

int main()
{
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  SimpleChessEngine::InitPawnAttacks();
  SimpleChessEngine::InitPSQT();

  SimpleChessEngine::UciChessEngine uci;
  uci.Start();
}