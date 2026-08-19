#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include "Attacks.h"
#include "PSQT.h"
#include "TablebaseFile.h"
#include "TablebasePosition.h"

using namespace SimpleChessEngine;
using namespace SimpleChessEngine::Tablebase;

int main(int argc, char** argv) {
  if (argc < 2 || argc > 4) {
    std::cerr << "usage: SCE_tbbench table.scetb [probes] [threads]\n";
    return 2;
  }
  const std::uint64_t probe_count = argc >= 3 ? std::stoull(argv[2]) : 10'000'000;
  const std::size_t thread_count = argc >= 4 ? std::stoull(argv[3]) : 1;
  if (probe_count == 0 || thread_count == 0) return 2;

  InitBetween<Piece::kBishop>();
  InitBetween<Piece::kRook>();
  InitPawnAttacks();
  InitPSQT();
  auto tablebase = MappedFile::Open(std::filesystem::path{argv[1]});
  if (!tablebase) return 1;

  std::vector<Position> samples;
  constexpr std::size_t kSampleCount = 4096;
  for (const auto material : tablebase->Materials()) {
    const auto raw_size = PositionIndexer::RawSize(material);
    for (std::uint64_t raw = 0;
         raw < raw_size && samples.size() < kSampleCount; ++raw) {
      if (!PositionBuilder::IsCanonicalLegalRaw(material, raw)) continue;
      const auto encoded = PositionIndexer::FromRawIndex(material, raw);
      auto position = encoded ? PositionBuilder::Build(*encoded) : std::nullopt;
      if (position) samples.push_back(std::move(*position));
    }
    if (samples.size() == kSampleCount) break;
  }
  if (samples.empty()) return 1;

  std::atomic<std::int64_t> checksum{};
  const auto started = std::chrono::steady_clock::now();
  std::vector<std::thread> workers;
  workers.reserve(thread_count);
  for (std::size_t thread = 0; thread < thread_count; ++thread) {
    workers.emplace_back([&, thread] {
      std::int64_t local_checksum = 0;
      const auto begin = probe_count * thread / thread_count;
      const auto end = probe_count * (thread + 1) / thread_count;
      for (auto index = begin; index < end; ++index) {
        const auto result = tablebase->ProbeWdl(samples[index % samples.size()]);
        if (!result) return;
        local_checksum += static_cast<std::int8_t>(*result);
      }
      checksum.fetch_add(local_checksum, std::memory_order_relaxed);
    });
  }
  for (auto& worker : workers) worker.join();
  const auto seconds = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - started).count();
  std::cout << "probes=" << probe_count << " threads=" << thread_count
            << " seconds=" << seconds
            << " probes_per_second=" << static_cast<std::uint64_t>(
                   static_cast<double>(probe_count) / seconds)
            << " checksum=" << checksum.load() << '\n';
  return 0;
}
