#pragma once
#include <cassert>
#include <chrono>
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

struct IterationInfo {
  const Searcher& searcher;
  Eval iteration_result;
  size_t depth;
};

template <class SearchCondition>
concept IsSearchCondition =
    requires(SearchCondition condition, IterationInfo info) {
      { condition.ShouldContinueIteration() } -> std::convertible_to<bool>;
      { condition.GetEndSearchTime() } -> std::convertible_to<TimePoint>;
      { condition.Update(info) };
    };

struct TimeCondition {
  TimeCondition(std::chrono::milliseconds time_for_move)
      : time_for_move_(std::move(time_for_move)) {}

  bool ShouldContinueIteration() const {
    return time_for_move_ > (std::chrono::system_clock::now() - start_time_);
  }

  TimePoint GetEndSearchTime() const { return time_for_move_ + start_time_; }

  void Update(const IterationInfo&) {}

  const std::chrono::milliseconds time_for_move_;
  const TimePoint start_time_ = std::chrono::system_clock::now();
};
static_assert(IsSearchCondition<TimeCondition>);

struct DepthCondition {
  DepthCondition(size_t max_depth) : max_depth_(max_depth) {}
  bool ShouldContinueIteration() const { return cur_depth < max_depth_; }

  TimePoint GetEndSearchTime() const { return TimePoint::max(); }

  void Update(const IterationInfo& info) { cur_depth = info.depth; }

  size_t cur_depth = 0;
  const size_t max_depth_;
};
static_assert(IsSearchCondition<DepthCondition>);

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

  void ComputeBestMove(const IsSearchCondition auto conditions);

  [[nodiscard]] const Move& GetCurrentBestMove() const;

  void PrintBestMove() { o_stream_ << BestMoveInfo{GetCurrentBestMove()}; }

  void PrintBestMove(const Move& move) { o_stream_ << BestMoveInfo{move}; }

 private:
  class EBFsInfo {
    void Update(std::size_t searched_nodes) {
      if (previous_nodes_ != 0) {
        ebfs_.push_back(static_cast<float>(searched_nodes) / previous_nodes_);

        float odd_even_average_ = 0;
        if (ebfs_.size() > 1) {
          for (auto it = ebfs_.rbegin(); ebfs_.rend() - it > 1; it += 2) {
            odd_even_average_ += *it;
          }
          odd_even_average_ /= ebfs_.size() / 2;
        }
      }
      odd_even_average_ = searched_nodes;
    }

    EBFInfo GetInfo() const {
      return EBFInfo{ebfs_.back(), odd_even_average_,
                     std::reduce(ebfs_.begin(), ebfs_.end()) / ebfs_.size()};
    }

    float odd_even_average_;
    std::vector<float> ebfs_;
    size_t previous_nodes_ = 0;
  };

  void PrintInfo(const Searcher::DebugInfo& info, Eval eval,
                 size_t current_depth,
                 std::chrono::duration<double> search_time) {
    PrintInfo(ScoreInfo{eval});
    PrincipalVariationInfo pv{current_depth,
                              searcher_.GetPrincipalVariation(current_depth)};
    PrintInfo(pv);
    PrintInfo(NodesInfo{info.searched_nodes});
    PrintInfo(NodesInfo{info.quiescence_nodes});
    PrintInfo(NodePerSecondInfo{static_cast<std::size_t>(
        (info.searched_nodes + info.quiescence_nodes) / search_time.count())});
  }

  template <class Info>
  void PrintInfo(const Info& info);

  std::optional<Eval> MakeIteration(std::size_t depth, const TimePoint& end);

  std::ostream& o_stream_;

  Searcher searcher_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
inline void SimpleChessEngine::ChessEngine::ComputeBestMove(
    IsSearchCondition auto condition) {
  const TimePoint start_time = std::chrono::system_clock::now();
  searcher_.InitStartOfSearch();

  EBFInfo ebfs;
  Searcher::DebugInfo info;

  Move best_move{};
  for (size_t current_depth = 1;
       condition.ShouldContinueIteration() && current_depth < kMaxSearchPly;
       ++current_depth) {
    PrintInfo(DepthInfo{current_depth});
    const auto eval_optional =
        MakeIteration(current_depth, condition.GetEndSearchTime());
    if (!eval_optional) {
      break;
    }
    best_move = GetCurrentBestMove();

    condition.Update(IterationInfo{searcher_, *eval_optional, current_depth});

    info += searcher_.GetInfo();
    PrintInfo(info, *eval_optional, current_depth,
              std::chrono::duration<double>{std::chrono::system_clock::now() -
                                            start_time});
  }

  PrintBestMove(best_move);
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
