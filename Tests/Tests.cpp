#include <gtest/gtest.h>

#include "Chess/Attacks.h"
#include "Chess/PSQT.h"

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  SimpleChessEngine::InitPawnAttacks();
  SimpleChessEngine::InitPSQT();

  return RUN_ALL_TESTS();
}