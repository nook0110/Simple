#include <atomic>
#include <bit>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

#include "Attacks.h"
#include "MoveGenerator.h"
#include "PSQT.h"
#include "PositionFactory.h"
#include "TablebaseFile.h"
#include "TablebasePosition.h"

using namespace SimpleChessEngine;
using namespace SimpleChessEngine::Tablebase;

int main(int argc, char** argv) {
  if (argc < 2 || argc > 4) {
    std::cerr << "usage: SCE_tbverify table.scetb [--threads n]\n";
    return 2;
  }
  std::size_t threads = std::thread::hardware_concurrency();
  if (argc == 4) {
    if (std::string_view{argv[2]} != "--threads") return 2;
    threads = std::stoul(argv[3]);
  }
  if (threads == 0) return 2;

  InitBetween<Piece::kBishop>();
  InitBetween<Piece::kRook>();
  InitPawnAttacks();
  InitPSQT();

  auto tablebase = MappedFile::Open(std::filesystem::path{argv[1]});
  if (!tablebase || !tablebase->VerifyChecksums()) {
    std::cerr << "file/header/checksum verification failed\n";
    return 1;
  }

  std::atomic<std::uint64_t> verified{};
  std::atomic<std::uint64_t> terminal{};
  std::atomic<bool> failed{};
  std::atomic<std::uint16_t> max_dtz{};
  std::mutex max_dtz_mutex;
  std::string max_dtz_fen;
  std::mutex failure_mutex;
  std::string failure_reason;
  for (const auto material : tablebase->Materials()) {
    const auto raw_size = PositionIndexer::RawSize(material);
    std::atomic<std::uint64_t> next_raw{};
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (std::size_t thread = 0; thread < threads; ++thread) {
      workers.emplace_back([&] {
        thread_local MoveGenerator generator;
        while (!failed.load(std::memory_order_relaxed)) {
          const auto raw_index =
              next_raw.fetch_add(1, std::memory_order_relaxed);
          if (raw_index >= raw_size) return;
          if (!PositionBuilder::IsCanonicalLegalRaw(material, raw_index)) {
            continue;
          }
          const auto encoded = PositionIndexer::FromRawIndex(material, raw_index);
          auto position = encoded ? PositionBuilder::Build(*encoded)
                                  : std::nullopt;
          if (!position) {
            failed.store(true, std::memory_order_relaxed);
            return;
          }
          const auto result = tablebase->Probe(*position);
          if (!result) {
            failed.store(true, std::memory_order_relaxed);
            return;
          }
          const auto distance = static_cast<std::uint16_t>(
              std::abs(static_cast<std::int32_t>(result->dtz)));
          auto maximum = max_dtz.load(std::memory_order_relaxed);
          while (maximum < distance &&
                 !max_dtz.compare_exchange_weak(
                     maximum, distance, std::memory_order_relaxed)) {
          }
          if (distance == max_dtz.load(std::memory_order_relaxed)) {
            std::lock_guard lock(max_dtz_mutex);
            if (distance == max_dtz.load(std::memory_order_relaxed)) {
              max_dtz_fen = FenFactory{}(*position);
            }
          }
          const auto moves =
              generator.GenerateMoves<MoveGenerator::Type::kAll>(*position);
          if (moves.empty()) {
            const auto expected_wdl = position->IsUnderCheck()
                                          ? Wdl::kLoss
                                          : Wdl::kDraw;
            const auto expected_dtz =
                expected_wdl == Wdl::kLoss ? -1 : 0;
            if (result->wdl != expected_wdl || result->dtz != expected_dtz) {
              failed.store(true, std::memory_order_relaxed);
              return;
            }
            terminal.fetch_add(1, std::memory_order_relaxed);
          } else if (position->IsInsufficientMaterial() &&
                     (result->wdl != Wdl::kDraw || result->dtz != 0)) {
            failed.store(true, std::memory_order_relaxed);
            return;
          } else if (!position->IsInsufficientMaterial()) {
            int best_child_result = -2;
            for (const auto move : moves) {
              const auto irreversible = position->GetIrreversibleData();
              position->DoMove(move);
              const auto child = tablebase->ProbeWdl(*position);
              const auto child_fen = child ? std::string{} : FenFactory{}(*position);
              position->UndoMove(move, irreversible);
              if (!child) {
                std::lock_guard lock(failure_mutex);
                std::ostringstream move_text;
                move_text << move;
                failure_reason = "missing child move=" + move_text.str() +
                                 " parent=" + FenFactory{}(*position) +
                                 " child=" + child_fen;
                failed.store(true, std::memory_order_relaxed);
                return;
              }
              const auto child_sign =
                  (static_cast<std::int8_t>(*child) > 0) -
                  (static_cast<std::int8_t>(*child) < 0);
              best_child_result = std::max(best_child_result, -child_sign);
            }
            const auto stored_sign =
                (static_cast<std::int8_t>(result->wdl) > 0) -
                (static_cast<std::int8_t>(result->wdl) < 0);
            if (stored_sign != best_child_result) {
              std::lock_guard lock(failure_mutex);
              failure_reason =
                  "WDL recurrence mismatch at " + FenFactory{}(*position) +
                  " stored=" + std::to_string(stored_sign) +
                  " expected=" + std::to_string(best_child_result);
              failed.store(true, std::memory_order_relaxed);
              return;
            }
          }
          verified.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }
    for (auto& worker : workers) worker.join();
    if (failed.load()) break;
  }

  if (failed.load()) {
    std::cerr << "semantic verification failed after " << verified.load()
              << " positions: " << failure_reason << '\n';
    return 1;
  }
  std::cout << "verified=" << verified.load()
            << " terminal=" << terminal.load()
            << " classes=" << tablebase->ClassCount()
            << " max_dtz=" << max_dtz.load()
            << " max_dtz_fen=\"" << max_dtz_fen << "\"\n";
  return 0;
}
