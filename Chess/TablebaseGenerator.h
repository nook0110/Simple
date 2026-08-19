#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <vector>

#include "TablebaseFile.h"

namespace SimpleChessEngine {
class Position;

namespace Tablebase {

class GeneratedDatabase {
 public:
  [[nodiscard]] std::optional<ProbeResult> Probe(
      const Position& position) const;
  [[nodiscard]] bool Write(const std::filesystem::path& path) const;
  [[nodiscard]] std::span<const ClassData> Classes() const { return classes_; }

 private:
  friend class Generator;
  [[nodiscard]] std::optional<std::pair<std::size_t, std::uint64_t>> Locate(
      const Position& position) const;

  std::vector<ClassData> classes_;
  std::array<std::int16_t, 4096> class_lookup_{};
};

class Generator {
 public:
  using Progress = std::function<void(std::string_view, std::uint32_t)>;
  struct Stats {
    std::uint64_t legal_positions{};
    std::uint64_t terminal_losses{};
    std::uint64_t terminal_draws{};
    std::uint64_t resolved_wins{};
    std::uint64_t resolved_losses{};
    std::uint64_t unresolved_draws{};
    std::uint32_t wdl_iterations{};
    std::uint32_t dtz_iterations{};
    std::uint16_t max_dtz{};
  };

  [[nodiscard]] static std::optional<GeneratedDatabase> GenerateWdl(
      std::span<const MaterialClass> materials, std::size_t threads,
      Stats* stats = nullptr, Progress progress = {});
  [[nodiscard]] static std::optional<GeneratedDatabase> GenerateLayout(
      std::span<const MaterialClass> materials, std::size_t threads,
      Progress progress = {});
};

}  // namespace Tablebase
}  // namespace SimpleChessEngine
