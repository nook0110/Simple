#include <charconv>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

#include "Attacks.h"
#include "PSQT.h"
#include "Tablebase.h"
#include "TablebaseGenerator.h"

using namespace SimpleChessEngine::Tablebase;

namespace {

std::optional<std::uint32_t> ParseUnsigned(const std::string_view value) {
  std::uint32_t result{};
  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), result);
  if (error != std::errc{} || end != value.data() + value.size()) {
    return std::nullopt;
  }
  return result;
}

}  // namespace

int main(int argc, char** argv) {
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kBishop>();
  SimpleChessEngine::InitBetween<SimpleChessEngine::Piece::kRook>();
  SimpleChessEngine::InitPawnAttacks();
  SimpleChessEngine::InitPSQT();

  std::filesystem::path output = "sce-4.scetb";
  std::uint32_t max_pieces = 4;
  std::uint32_t threads = std::thread::hardware_concurrency();
  bool layout_only = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view argument = argv[i];
    if (argument == "--output" && i + 1 < argc) {
      output = argv[++i];
    } else if (argument == "--max-pieces" && i + 1 < argc) {
      const auto parsed = ParseUnsigned(argv[++i]);
      if (!parsed) return 2;
      max_pieces = *parsed;
    } else if (argument == "--threads" && i + 1 < argc) {
      const auto parsed = ParseUnsigned(argv[++i]);
      if (!parsed) return 2;
      threads = *parsed;
    } else if (argument == "--layout-only") {
      layout_only = true;
    } else {
      std::cerr << "usage: SCE_tbgen [--output file] [--max-pieces 2..4] "
                   "[--threads n] [--layout-only]\n";
      return 2;
    }
  }
  if (max_pieces < 2 || max_pieces > 4 || threads == 0) return 2;

  auto materials = AllMaterialClasses();
  std::erase_if(materials, [max_pieces](const MaterialClass& material) {
    return material.PieceCount() > max_pieces;
  });
  if (materials.empty()) return 2;
  Generator::Stats stats;
  const auto progress =
      [class_count = materials.size()](const std::string_view phase,
                                       const std::uint32_t step) {
        std::cerr << "progress phase=" << phase << " step=" << step;
        if (phase == "layout") std::cerr << "/" << class_count;
        std::cerr << '\n';
      };
  const auto database =
      layout_only ? Generator::GenerateLayout(materials, threads, progress)
                  : Generator::GenerateWdl(materials, threads, &stats, progress);
  const auto temporary_output = output.string() + ".tmp";
  std::filesystem::remove(temporary_output);
  if (!database || !database->Write(temporary_output)) return 1;
  auto verification = MappedFile::Open(temporary_output);
  if (!verification || !verification->VerifyChecksums()) {
    std::filesystem::remove(temporary_output);
    return 1;
  }
  std::filesystem::rename(temporary_output, output);
  std::cout << "classes=" << database->Classes().size()
            << " positions=" << stats.legal_positions
            << " terminal_losses=" << stats.terminal_losses
            << " terminal_draws=" << stats.terminal_draws
            << " wins=" << stats.resolved_wins
            << " losses=" << stats.resolved_losses
            << " draws=" << stats.unresolved_draws
            << " wdl_iterations=" << stats.wdl_iterations
            << " dtz_iterations=" << stats.dtz_iterations
            << " max_dtz=" << stats.max_dtz << '\n';
  return 0;
}
