#pragma once
#include <cstddef>

namespace SimpleChessEngine {
struct DebugInfo {
  std::size_t searched_nodes{};
  std::size_t quiescence_nodes{};

  std::size_t zws_researches{};

  std::size_t rfp_cuts{};
  std::size_t nmp_tries{};
  std::size_t nmp_cuts{};

  std::size_t tt_hits{};
  std::size_t tt_cuts{};

  DebugInfo &operator+=(const DebugInfo &other) {
    searched_nodes += other.searched_nodes;
    quiescence_nodes += other.quiescence_nodes;
    zws_researches += other.zws_researches;
    rfp_cuts += other.rfp_cuts;
    nmp_tries += other.nmp_tries;
    nmp_cuts += other.nmp_cuts;
    tt_hits += other.tt_hits;
    tt_cuts += other.tt_cuts;
    return *this;
  }
};
}  // namespace SimpleChessEngine