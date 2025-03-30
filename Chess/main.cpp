#include "PSQT.h"
#include "UciCommunicator.h"

std::string program_info() {
  std::ostringstream ss;
  ss << "Welcome to SimpleChessEngine\n";
  ss << "Build date: " << __DATE__ << " " << __TIME__ << "\n";

  ss << std::boolalpha;
  ss << "PruneParameters:\n";
  using namespace SimpleChessEngine;
  ss << "rfp:\n";
  ss << "enabled: " << Searcher::PruneParameters::rfp::enabled << "\n";
  if constexpr (Searcher::PruneParameters::rfp::enabled) {
    ss << "depth_limit: "
       << static_cast<size_t>(Searcher::PruneParameters::rfp::depth_limit)
       << "\n";
    ss << "threshold: " << Searcher::PruneParameters::rfp::threshold << "\n";
  }
  return ss.str();
}

int main(int argc, char **argv) {
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  SimpleChessEngine::InitPawnAttacks();
  SimpleChessEngine::InitPSQT();

  std::cout << program_info() << std::endl;

  if (argc == 1) {
    SimpleChessEngine::UciChessEngine uci;
    uci.Start();
    return EXIT_SUCCESS;
  }
  std::stringstream ss;
  for (size_t arg_idx = 1; arg_idx < argc; ++arg_idx) {
    ss << argv[arg_idx] << "\n";
  }
  SimpleChessEngine::UciChessEngine uci(ss);
  uci.Start();
  return EXIT_SUCCESS;
}
//"position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq
//-" "go perft 6" "stop"