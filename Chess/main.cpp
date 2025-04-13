#include "PSQT.h"
#include "UciCommunicator.h"

std::string program_info() {
  using namespace SimpleChessEngine;
  std::ostringstream ss;
  ss << "Welcome to SimpleChessEngine\n";
  ss << "Build date: " << __DATE__ << " " << __TIME__ << "\n";
  ss << "Features:\n";
  ss << "\tAlpha-Beta function\n";
  ss << "\tQuiescence search\n";
  ss << "\tPSQT\n";
  ss << "\tMVV-LVA\n";
  ss << "\tTT\n";
  ss << "\tTT move ordering\n";
  ss << "\tPVS\n";
  ss << "\tHistory heuristics\n";
  ss << "\tKiller moves\n";
  ss << "\tTapered Evaluation\n";

  ss << std::boolalpha;
  ss << "PruneParameters:\n";
  ss << "\trfp: " << Searcher::PruneParameters::rfp::kEnabled << "\n";
  if constexpr (Searcher::PruneParameters::rfp::kEnabled) {
    ss << "\t\tkDepthLimit: "
       << static_cast<size_t>(Searcher::PruneParameters::rfp::kDepthLimit)
       << "\n";
    ss << "\t\tkThreshold: " << Searcher::PruneParameters::rfp::kThreshold
       << "\n";
  }
  ss << "\tNullMove: " << Searcher::PruneParameters::NullMove::kEnabled << "\n";
  if constexpr (Searcher::PruneParameters::NullMove::kEnabled) {
    ss << "\t\tkNullMoveReduction: "
       << static_cast<size_t>(
              Searcher::PruneParameters::NullMove::kNullMoveReduction)
       << "\n";
  }
  return ss.str();
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
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