#pragma once
#include <cassert>
#include <chrono>
#include <expected>
#include <variant>

#include "Evaluation.h"
#include "Move.h"
#include "Searcher.h"
#include "numeric"

namespace SimpleChessEngine {
struct DepthInfo {
  size_t current_depth = 0;
};

struct ScoreInfo {
  Eval current_eval = Eval{};
};

struct NodesInfo {
  size_t nodes = 0;
};

struct NodePerSecondInfo {
  size_t nodes_per_second = 0;
};

struct PrincipalVariationInfo {
  size_t current_depth = 0;
  MoveGenerator::Moves moves;
};

struct BestMoveInfo {
  const Move& move;
};

struct TranspositionTableInfo {
  std::size_t tt_hits{};
};

struct EBFInfo {
  float last_ebf;
  float avg_odd_even_ebf;
  float avg_ebf;
};

std::ostream& operator<<(std::ostream& out, const DepthInfo& depth_info);
std::ostream& operator<<(std::ostream& out, const ScoreInfo& score_info);

std::ostream& operator<<(std::ostream& out, const NodesInfo& nodes_info);

std::ostream& operator<<(std::ostream& out, const NodePerSecondInfo& nps_info);
std::ostream& operator<<(std::ostream& out,
                         const PrincipalVariationInfo& principal_variation);
std::ostream& operator<<(std::ostream& out,
                         const TranspositionTableInfo& tt_info);
std::ostream& operator<<(std::ostream& out, const BestMoveInfo& bm_info);
std::ostream& operator<<(std::ostream& out, const EBFInfo& ebf_info);

/**
 * \brief Class that represents a chess engine.
 *
 * \author nook0110
 */
class ChessEngine {
 public:
  explicit ChessEngine(const Position position = PositionFactory{}(),
                       std::ostream& o_stream = std::cout)
      : searcher_(position), o_stream_(o_stream) {}

  void SetPosition(const Position position) { searcher_.SetPosition(position); }

  void ComputeBestMove(size_t depth);

  void ComputeBestMove(const std::chrono::milliseconds left_time,
                       const std::chrono::milliseconds inc_time);

  [[nodiscard]] const Move& GetCurrentBestMove() const;

  void PrintBestMove() { o_stream_ << BestMoveInfo{GetCurrentBestMove()}; }

  void PrintBestMove(const Move& move) { o_stream_ << BestMoveInfo{move}; }

 private:
  template <class Info>
  void PrintInfo(const Info& info);

  std::optional<Eval> MakeIteration(std::size_t depth, const TimePoint& end);

  std::ostream& o_stream_;

  Searcher searcher_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
inline void ChessEngine::ComputeBestMove(const size_t depth) { assert(false); }

inline void ChessEngine::ComputeBestMove(
    const std::chrono::milliseconds left_time,
    const std::chrono::milliseconds inc_time = {}) {
  const TimePoint start_time = std::chrono::system_clock::now();
  constexpr size_t kAverageGameLength = 40;

  std::chrono::milliseconds time_for_move =
      left_time / kAverageGameLength + inc_time;
  time_for_move = std::min(left_time / 2, time_for_move);

  Searcher::DebugInfo info{};

  searcher_.InitStartOfSearch();

  std::vector<float> ebfs;
  ebfs.reserve(kMaxSearchPly);
  size_t previous_nodes = 0;

  Move previous_best_move{};
  size_t last_best_move_change{};
  for (size_t current_depth = 1;
       time_for_move > (std::chrono::system_clock::now() - start_time);
       ++current_depth) {
    PrintInfo(DepthInfo{current_depth});
    const auto eval_optional =
        MakeIteration(current_depth, time_for_move + start_time);
    if (!eval_optional) {
      PrintBestMove(previous_best_move);
      return;
    }
    info += searcher_.GetInfo();

    PrintInfo(ScoreInfo{*eval_optional});

    // check if best move changed
    if (previous_best_move == searcher_.GetCurrentBestMove()) {
      // increase last change
      ++last_best_move_change;
    } else {
      // reset last change
      last_best_move_change = 0;
    }

    previous_best_move = GetCurrentBestMove();

    PrincipalVariationInfo pv{current_depth,
                              searcher_.GetPrincipalVariation(current_depth)};
    PrintInfo(pv);
    PrintInfo(NodesInfo{info.searched_nodes});

    if (previous_nodes != 0) {
      ebfs.push_back(static_cast<float>(info.searched_nodes) / previous_nodes);

      float odd_even_average = 0;
      if (ebfs.size() > 1) {
        auto it = ebfs.rbegin();
        while (it < ebfs.rend()) {
          odd_even_average += *it;
          if (ebfs.rend() - it < 2) break;
          it += 2;
        }
        odd_even_average /= ebfs.size() / 2;
      }
      PrintInfo(EBFInfo{ebfs.back(), odd_even_average,
                        std::reduce(ebfs.begin(), ebfs.end()) / ebfs.size()});
    }
    previous_nodes = info.searched_nodes;

    PrintInfo(NodesInfo{info.quiescence_nodes});
    PrintInfo(NodePerSecondInfo{static_cast<std::size_t>(
        (info.searched_nodes + info.quiescence_nodes) /
        (std::chrono::duration<double>{std::chrono::system_clock::now() -
                                       start_time})
            .count())});
    info = Searcher::DebugInfo{};
  }

  PrintBestMove(previous_best_move);
}

inline const Move& ChessEngine::GetCurrentBestMove() const {
  return searcher_.GetCurrentBestMove();
}

inline std::optional<Eval> ChessEngine::MakeIteration(
    const std::size_t current_depth, const TimePoint& end) {
  constexpr auto neg_inf = std::numeric_limits<Eval>::min() / 2;
  constexpr auto pos_inf = std::numeric_limits<Eval>::max() / 2;

  return searcher_.Search<true>(end, current_depth, current_depth, neg_inf,
                                pos_inf);
}

template <class Info>
void ChessEngine::PrintInfo(const Info& info) {
  o_stream_ << info;
}

inline std::ostream& operator<<(std::ostream& out,
                                const DepthInfo& depth_info) {
  return out << "info depth " << depth_info.current_depth << std::endl;
}

inline std::ostream& operator<<(std::ostream& out,
                                const ScoreInfo& score_info) {
  const auto& eval = score_info.current_eval;
  if (!IsMateScore(eval)) {
    return out << "info score cp " << eval << std::endl;
  }

  return out << "info score mate "
             << IsMateScore(eval) * (-kMateValue - std::abs(eval) + 1) / 2
             << std::endl;
}

inline std::ostream& operator<<(std::ostream& out,
                                const NodesInfo& nodes_info) {
  return out << "info nodes " << nodes_info.nodes << std::endl;
}

inline std::ostream& operator<<(std::ostream& out,
                                const NodePerSecondInfo& nps_info) {
  return out << "info nps " << nps_info.nodes_per_second << std::endl;
}

inline std::ostream& operator<<(
    std::ostream& out, const PrincipalVariationInfo& principal_variation) {
  out << "info depth " << principal_variation.moves.size() << " pv";
  for (const auto move : principal_variation.moves) {
    out << " " << move;
  }
  return out << std::endl;
}

inline std::ostream& operator<<(std::ostream& out,
                                const TranspositionTableInfo& tt_info) {
  return out << "info tt_hits " << tt_info.tt_hits << std::endl;
}

inline std::ostream& operator<<(std::ostream& out,
                                const BestMoveInfo& bm_info) {
  return out << "bestmove " << bm_info.move << std::endl;
}

inline std::ostream& operator<<(std::ostream& out, const EBFInfo& ebf_info) {
  out << "info last ebf " << ebf_info.last_ebf << std::endl;
  out << "info avg odd-even-ebf " << ebf_info.avg_odd_even_ebf << std::endl;
  out << "info avg ebf " << ebf_info.avg_ebf << std::endl;
  return out;
}
}  // namespace SimpleChessEngine
