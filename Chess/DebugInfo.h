#pragma once
#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace SimpleChessEngine {
struct DebugInfo {
  std::size_t searched_nodes{};
  std::size_t quiescence_nodes{};

  std::size_t zws_researches{};

  std::size_t tt_hits{};
  std::size_t tt_cuts{};

  std::size_t rfp_cuts{};
  std::size_t nmp_tries{};
  std::size_t nmp_cuts{};

  std::size_t lmp_prunes{};
  std::size_t see_prunes{};
  std::size_t futility_prunes{};

  DebugInfo &operator+=(const DebugInfo &other);
};

using DebugCounter = std::pair<std::string_view, std::size_t DebugInfo::*>;

inline constexpr auto kDebugCounters = std::to_array<DebugCounter>({
    {"searched_nodes", &DebugInfo::searched_nodes},
    {"quiescence_nodes", &DebugInfo::quiescence_nodes},
    {"zws_researches", &DebugInfo::zws_researches},
    {"tt_hits", &DebugInfo::tt_hits},
    {"tt_cuts", &DebugInfo::tt_cuts},
    {"rfp_cuts", &DebugInfo::rfp_cuts},
    {"nmp_tries", &DebugInfo::nmp_tries},
    {"nmp_cuts", &DebugInfo::nmp_cuts},
    {"lmp_prunes", &DebugInfo::lmp_prunes},
    {"see_prunes", &DebugInfo::see_prunes},
    {"futility_prunes", &DebugInfo::futility_prunes},
});

static_assert(sizeof(DebugInfo) == kDebugCounters.size() * sizeof(std::size_t));

inline DebugInfo &DebugInfo::operator+=(const DebugInfo &other) {
  for (const auto &[_, counter] : kDebugCounters) {
    this->*counter += other.*counter;
  }
  return *this;
}
}  // namespace SimpleChessEngine
