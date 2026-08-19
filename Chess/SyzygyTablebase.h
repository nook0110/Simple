#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>

#include "Move.h"
#include "Tablebase.h"

namespace SimpleChessEngine {
class Position;

namespace Tablebase {

class Syzygy {
 public:
  [[nodiscard]] static std::shared_ptr<const Syzygy> Open(
      const std::filesystem::path& path);
  static void Disable();

  [[nodiscard]] std::optional<ProbeResult> Probe(
      const Position& position) const;
  [[nodiscard]] std::optional<Wdl> ProbeWdl(const Position& position) const;
  [[nodiscard]] std::optional<Move> RootMove(const Position& position) const;
  [[nodiscard]] std::size_t MaxPieces() const { return max_pieces_; }

 private:
  explicit Syzygy(const std::size_t max_pieces) : max_pieces_(max_pieces) {}

  [[nodiscard]] bool CanProbe(const Position& position) const;

  std::size_t max_pieces_{};
  static std::mutex initialization_mutex_;
  static std::mutex root_probe_mutex_;
};

}  // namespace Tablebase
}  // namespace SimpleChessEngine
