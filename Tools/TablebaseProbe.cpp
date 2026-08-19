#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include "Attacks.h"
#include "PSQT.h"
#include "PositionFactory.h"
#include "SyzygyTablebase.h"

using namespace SimpleChessEngine;

int main(int argc, char** argv) {
  InitBetween<Piece::kBishop>();
  InitBetween<Piece::kRook>();
  InitPawnAttacks();
  InitPSQT();
  if (argc < 3) {
    std::cerr << "usage: SCE_tbprobe syzygy-path fen\n";
    return 2;
  }

  std::ostringstream fen;
  for (int i = 2; i < argc; ++i) {
    if (fen.tellp() > 0) fen << ' ';
    fen << argv[i];
  }
  const auto tablebase =
      Tablebase::Syzygy::Open(std::filesystem::path{argv[1]});
  if (!tablebase) {
    std::cerr << "failed to open Syzygy tablebases\n";
    return 1;
  }
  const auto position = PositionFactory{}(fen.str());
  const auto result = tablebase->Probe(position);
  if (!result) {
    std::cout << "unavailable\n";
    return 3;
  }
  std::cout << "wdl=" << static_cast<int>(result->wdl)
            << " dtz=" << result->dtz;
  if (const auto move = tablebase->RootMove(position)) {
    std::cout << " move=" << *move;
  }
  std::cout << '\n';
  return 0;
}
