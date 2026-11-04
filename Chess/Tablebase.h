#pragma once

#include <cstdint>

namespace SimpleChessEngine::Tablebase {

enum class Wdl : std::int8_t {
  kLoss = -2,
  kBlessedLoss = -1,
  kDraw = 0,
  kCursedWin = 1,
  kWin = 2,
};

struct ProbeResult {
  Wdl wdl{Wdl::kDraw};
  std::int16_t dtz{};

  bool operator==(const ProbeResult&) const = default;
};

}  // namespace SimpleChessEngine::Tablebase
