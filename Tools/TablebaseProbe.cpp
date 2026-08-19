#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include "Attacks.h"
#include "PSQT.h"
#include "PositionFactory.h"
#include "TablebaseFile.h"
#include "TablebasePosition.h"

using namespace SimpleChessEngine;
using namespace SimpleChessEngine::Tablebase;

int main(int argc, char** argv) {
  InitBetween<Piece::kBishop>();
  InitBetween<Piece::kRook>();
  InitPawnAttacks();
  InitPSQT();
  if (argc < 3) {
    std::cerr << "usage: SCE_tbprobe table.scetb fen [--verify]\n";
    return 2;
  }
  const std::filesystem::path path = argv[1];
  bool verify = false;
  bool debug = false;
  std::ostringstream fen;
  for (int i = 2; i < argc; ++i) {
    if (std::string_view{argv[i]} == "--verify") {
      verify = true;
      continue;
    }
    if (std::string_view{argv[i]} == "--debug") {
      debug = true;
      continue;
    }
    if (fen.tellp() > 0) fen << ' ';
    fen << argv[i];
  }

  auto tablebase = MappedFile::Open(path);
  if (!tablebase) {
    std::cerr << "failed to open tablebase\n";
    return 1;
  }
  if (verify && !tablebase->VerifyChecksums()) {
    std::cerr << "checksum verification failed\n";
    return 1;
  }
  const auto position = PositionFactory{}(fen.str());
  if (debug) {
    const auto canonical = PositionIndexer::Canonicalize(position);
    if (!canonical) {
      std::cout << "canonical=unavailable\n";
    } else {
      const auto raw_index = PositionIndexer::RawIndex(*canonical);
      const auto decoded =
          PositionIndexer::FromRawIndex(canonical->material, raw_index);
      const auto rebuilt = PositionBuilder::Build(*canonical);
      const auto recanonical =
          rebuilt ? PositionIndexer::Canonicalize(*rebuilt) : std::nullopt;
      std::cout << "material_key=" << MaterialKey(canonical->material)
                << " raw_index=" << raw_index
                << " canonical_legal="
                << PositionBuilder::IsCanonicalLegalRaw(canonical->material,
                                                        raw_index)
                << " build_error="
                << static_cast<int>(PositionBuilder::Diagnose(*canonical))
                << " file_contains="
                << tablebase->Contains(canonical->material, raw_index);
      if (recanonical) {
        std::cout << " recanonical_key=" << MaterialKey(recanonical->material)
                  << " recanonical_raw="
                  << PositionIndexer::RawIndex(*recanonical);
      }
      if (decoded) {
        std::cout << " decoded=";
        for (std::uint8_t i = 0; i < decoded->material.PieceCount(); ++i) {
          std::cout << static_cast<int>(decoded->squares[i]) << ',';
        }
        std::cout << " decoded_stm="
                  << static_cast<int>(decoded->side_to_move);
      }
      if (rebuilt) std::cout << " rebuilt_fen=\"" << FenFactory{}(*rebuilt) << "\"";
      if (rebuilt) {
        const auto trace = PositionIndexer::Trace(*rebuilt);
        if (trace) {
          std::cout << " trace_original_key="
                    << MaterialKey(trace->original_material)
                    << " trace_original_raw="
                    << PositionIndexer::RawIndex(trace->original_geometry)
                    << " trace_swapped_key="
                    << MaterialKey(trace->swapped_material)
                    << " trace_swapped_raw="
                    << PositionIndexer::RawIndex(trace->swapped_geometry);
        }
      }
      std::cout << " flip_wp=" << static_cast<int>(
                       FlipColor(PieceCode::kWhitePawn))
                << " flip_bq=" << static_cast<int>(
                       FlipColor(PieceCode::kBlackQueen));
      std::cout << '\n';
    }
  }
  const auto result = tablebase->Probe(position);
  if (!result) {
    std::cout << "unavailable\n";
    return 3;
  }
  std::cout << "wdl=" << static_cast<int>(result->wdl)
            << " dtz=" << result->dtz << '\n';
  return 0;
}
