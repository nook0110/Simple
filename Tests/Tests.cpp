#include <gtest/gtest.h>

#include "Attacks.h"
#include "PSQT.h"

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  SimpleChessEngine::InitPawnAttacks();
  SimpleChessEngine::InitPSQT();

  return RUN_ALL_TESTS();
}