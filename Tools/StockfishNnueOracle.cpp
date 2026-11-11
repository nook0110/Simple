#include <iostream>
#include <memory>
#include <string>

#include "attacks.h"
#include "evaluate.h"
#include "nnue/network.h"
#include "nnue/nnue_accumulator.h"
#include "position.h"
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  Stockfish::Attacks::init();
  Stockfish::Position::init();
  auto network = std::make_unique<Stockfish::Eval::NNUE::Network>();
  Stockfish::Eval::NNUE::EvalFile file;
  network->load_external("", argv[1], file);
  if (!file.current) return 3;
  auto stack = std::make_unique<Stockfish::Eval::NNUE::AccumulatorStack>();
  auto cache =
      std::make_unique<Stockfish::Eval::NNUE::AccumulatorCaches>(*network);
  for (std::string fen; std::getline(std::cin, fen);) {
    Stockfish::Position position;
    Stockfish::StateInfo state;
    position.set(fen, false, &state);
    stack->reset();
    auto [psqt, positional] = network->evaluate(position, *stack, *cache);
    std::cout << psqt << ' ' << positional << ' ';
    if (!position.checkers())
      std::cout << Stockfish::Eval::evaluate(*network, position, *stack, *cache,
                                             0);
    else
      std::cout << "check";
    std::cout << '\n';
  }
}
