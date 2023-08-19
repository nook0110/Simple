#include "Attacks.h"
#include "SimpleChessEngine.h"
#include "UciCommunicator.h"

int main()
{
  auto pos = SimpleChessEngine::PositionFactory{}();

  SimpleChessEngine::UciChessEngine uci;
  uci.Start();
}