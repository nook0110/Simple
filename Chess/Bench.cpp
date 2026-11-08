#include "Bench.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

#include "PositionFactory.h"
#include "SimpleChessEngine.h"

namespace SimpleChessEngine {
namespace {
constexpr std::array kPositions = {
    "rn1qkbnr/pb3ppp/1p1p4/2p1p3/2B1P3/2NP1N2/PPPB1PPP/R2QK2R b KQkq - 0 1",
    "r1b1kb1r/pp3ppp/2n5/2pqp3/7N/3P4/PPP1BPPP/R1BQK2R b KQkq - 0 1",
    "rn2kb1r/pp1qpppp/2p2n2/4N3/6b1/2NP4/PPP1BPPP/R1BQ1RK1 b kq - 0 1",
    "r1bqk1nr/pppp1pbp/4p1p1/8/3nP3/2NBB3/PPP1NPPP/R2QK2R b KQkq - 0 1",
    "rnbqkbnr/1p1p1ppp/8/p2Pp3/4P3/p1P5/5PPP/RNBQKBNR b KQkq - 0 1",
    "r1bqk1nr/ppp1b1pp/2nP4/5p2/8/4P3/PPP1BPPP/RNBQK1NR b KQkq - 0 1",
    "r2qk2r/ppp1bppp/2np1n2/4pP2/2b1P3/3P1N2/PPPN2PP/R1BQK2R w KQkq - 0 1",
    "rnbq1rk1/ppp2pbp/3p1np1/4p3/3P3P/1P3NP1/P1P1PPB1/RNBQ1RK1 b - - 0 1",
    "rn2kbnr/pbpq1ppp/1p1P4/3p4/8/2N2N2/PPP2PPP/R1BQKB1R b KQkq - 0 1",
    "r1bqkb1r/pp3ppp/2np4/2p5/2P1n3/PP2PN2/1B3PPP/RN1QKB1R b KQkq - 0 1",
    "rnbqk1nr/ppp3bp/3pp1p1/4Pp2/8/5N2/PPPPBPPP/RNBQ1RK1 w kq - 0 1",
    "rn1qk1nr/ppp3bp/3pbpp1/4p3/8/1PN1PP2/PBPPQ1PP/2KR1BNR b kq - 0 1",
    "rn2kb1r/p1p2ppp/1p2pn2/8/3P4/4PN2/PP3PPP/R1B1KB1R w KQkq - 0 1",
    "r3kb1r/pppqpppp/2np1n2/8/8/3PP1Pb/PPPNNPBP/R1BQ1RK1 b kq - 0 1",
    "r2qkb1r/pp3ppp/2n1bn2/2p1p3/3pP3/P2P3P/1PPBBPP1/RN1QK1NR w KQkq - 0 1",
    "rnbq1rk1/p4ppp/1ppb1n2/3p4/3PpN2/2P1P1P1/PP3PBP/RNBQ1RK1 w - - 0 1",
    "rnb1kb1r/pp3ppp/3p1n2/2P5/8/5N2/PPP1qPPP/RNB1KB1R w KQkq - 0 1",
    "rn1qkbnr/1p2pppp/p7/2p5/3pPPb1/2P2N2/PP1PN1PP/R1BQKB1R w KQkq - 0 1",
    "r1b2rk1/p1pqppbp/1pnp1np1/8/2P5/2N1P1P1/PP1PNPBP/1RBQ1RK1 w - - 0 1",
    "r3k2r/1pp1qppp/p1nbbn2/8/8/P4NP1/1PP1PPBP/RNBQ1RK1 w kq - 0 1",
    "rnbqkb1r/ppp2p2/4pn2/3pN2p/3P2pP/4PPB1/PPP3P1/RN1QKB1R b KQkq - 0 1",
    "rnbqk1nr/pp3ppp/2p1p3/3p4/1bPPPP2/5N2/PP4PP/RNBQKB1R w KQkq - 0 1",
    "r2qkbnr/pp1b2pp/2n1p3/2ppP3/3P2P1/2P2N2/PP5P/RNBQKB1R b KQkq - 0 1",
    "rn1qkbnr/pp1b1ppp/4p3/2ppP3/8/2P4N/PP1P1PPP/RNBQKB1R w KQkq - 0 1",
    "r1bqk2r/pp1n2pp/2pbp3/3p4/2P3n1/5NP1/PP2PPBP/RNBQ1RK1 w kq - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "1k6/1P6/8/8/8/8/6p1/3K4 w - - 0 1",
    "4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1",
    "4k3/8/8/3pP3/4K3/8/8/8 w - d6 0 1",
    "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
    "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1",
    "4k3/8/8/8/1b6/8/4r3/4K3 w - - 0 1",
    "4k3/8/8/8/8/8/8/rRK5 w B - 0 1",
    "8/5pk1/3p2p1/2pPp3/2P1P1P1/5PK1/8/8 w - - 0 1",
    "7k/8/8/8/3pP3/8/8/K7 b - e3 37 42",
    "8/8/8/3k4/8/4K3/4P3/8 w - - 0 1",
};
}

void RunSearchBench(std::ostream& output, int depth) {
  std::ostringstream search_output;
  ChessEngine engine(PositionFactory{}(), search_output);
  engine.NewGame();
  std::uint64_t nodes = 0;
  const auto start = std::chrono::steady_clock::now();
  for (const auto* fen : kPositions) {
    engine.SetPosition(PositionFactory{}(fen));
    DepthCondition condition(depth);
    engine.ComputeBestMove(condition);
    std::istringstream lines(search_output.str());
    std::string line;
    std::uint64_t main_nodes = 0;
    std::uint64_t quiescence_nodes = 0;
    while (std::getline(lines, line)) {
      std::istringstream fields(line);
      std::string first;
      std::string second;
      fields >> first >> second;
      if (first == "info" && second == "nodes") fields >> main_nodes;
      if (first == "info" && second == "quiescence_nodes")
        fields >> quiescence_nodes;
    }
    nodes += main_nodes + quiescence_nodes;
    search_output.str({});
    search_output.clear();
  }
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
  output << "Nodes searched: " << nodes << '\n'
         << "Time (ms): " << elapsed << '\n'
         << "Nodes/second: " << (elapsed > 0 ? nodes * 1000 / elapsed : 0)
         << std::endl;
}
}
