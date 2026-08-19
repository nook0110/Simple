#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "Attacks.h"
#include "MoveGenerator.h"
#include "PSQT.h"
#include "PositionFactory.h"
#include "SyzygyTablebase.h"

using namespace SimpleChessEngine;

namespace {

std::string KqkFen(const int white_king, const int black_king,
                   const int white_queen, const bool white_to_move) {
  std::array<char, 64> board{};
  board[white_king] = 'K';
  board[black_king] = 'k';
  board[white_queen] = 'Q';
  std::ostringstream fen;
  for (int rank = 7; rank >= 0; --rank) {
    int empty = 0;
    for (int file = 0; file < 8; ++file) {
      const char piece = board[rank * 8 + file];
      if (piece == 0) {
        ++empty;
      } else {
        if (empty != 0) fen << empty;
        empty = 0;
        fen << piece;
      }
    }
    if (empty != 0) fen << empty;
    if (rank != 0) fen << '/';
  }
  fen << (white_to_move ? " w - - 0 1" : " b - - 0 1");
  return fen.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 4) {
    std::cerr << "usage: SCE_tbbench syzygy-path [probes] [threads]\n";
    return 2;
  }
  const std::uint64_t probes = argc >= 3 ? std::stoull(argv[2]) : 10'000'000;
  const std::size_t threads = argc >= 4 ? std::stoull(argv[3]) : 1;
  if (probes == 0 || threads == 0) return 2;

  InitBetween<Piece::kBishop>();
  InitBetween<Piece::kRook>();
  InitPawnAttacks();
  InitPSQT();
  const auto tablebase =
      Tablebase::Syzygy::Open(std::filesystem::path{argv[1]});
  if (!tablebase) return 1;
  std::vector<Position> samples;
  constexpr std::size_t kSampleCount = 4096;
  samples.reserve(kSampleCount);
  for (int white_king = 0;
       white_king < 64 && samples.size() < kSampleCount; ++white_king) {
    for (int black_king = 0;
         black_king < 64 && samples.size() < kSampleCount; ++black_king) {
      if (black_king == white_king) continue;
      for (int queen = 0; queen < 64 && samples.size() < kSampleCount;
           ++queen) {
        if (queen == white_king || queen == black_king) continue;
        for (const bool white_to_move : {false, true}) {
          auto position = PositionFactory{}(
              KqkFen(white_king, black_king, queen, white_to_move));
          if (position.IsUnderCheck(Flip(position.GetSideToMove()))) {
            continue;
          }
          const auto moves =
              MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
          const auto root_move = tablebase->RootMove(position);
          if (tablebase->ProbeWdl(position) && root_move &&
              std::ranges::find(moves, *root_move) != moves.end()) {
            samples.push_back(std::move(position));
          }
          if (samples.size() == kSampleCount) break;
        }
      }
    }
  }
  if (samples.size() != kSampleCount) return 1;

  std::atomic<std::int64_t> checksum{};
  const auto started = std::chrono::steady_clock::now();
  std::vector<std::thread> workers;
  workers.reserve(threads);
  for (std::size_t thread = 0; thread < threads; ++thread) {
    workers.emplace_back([&, thread] {
      std::int64_t local = 0;
      const auto begin = probes * thread / threads;
      const auto end = probes * (thread + 1) / threads;
      for (auto index = begin; index < end; ++index) {
        const auto result =
            tablebase->ProbeWdl(samples[index % samples.size()]);
        if (!result) return;
        local += static_cast<std::int8_t>(*result);
      }
      checksum.fetch_add(local, std::memory_order_relaxed);
    });
  }
  for (auto& worker : workers) worker.join();
  const auto seconds = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - started).count();
  std::cout << "probes=" << probes << " threads=" << threads
            << " seconds=" << seconds
            << " probes_per_second=" << static_cast<std::uint64_t>(
                   static_cast<double>(probes) / seconds)
            << " checksum=" << checksum.load() << '\n';
  return 0;
}
