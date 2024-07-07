#pragma once
#include <cassert>
#include <chrono>
#include <numeric>
#include <variant>

#include "Evaluation.h"
#include "Move.h"
#include "Searcher.h"

namespace SimpleChessEngine {
struct DepthInfo {
  Depth current_depth = 0;
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
  std::optional<Move> ponder;
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

template <class T>
concept SearchCondition =
    StopSearchCondition<T> && requires(T condition, IterationInfo info) {
      { condition.ShouldContinueIteration() } -> std::convertible_to<bool>;
      { condition.Update(info) };
    };

struct TimeCondition {
  explicit TimeCondition(std::chrono::milliseconds time_for_move)
      : time_for_move_(std::move(time_for_move)) {}

  bool ShouldContinueIteration() const { return !IsTimeToExit(); }

  bool IsTimeToExit() const {
    return time_for_move_ < (std::chrono::system_clock::now() - start_time_);
  }

  void Update(const IterationInfo&) const {}

  std::chrono::milliseconds time_for_move_;
  TimePoint start_time_ = std::chrono::system_clock::now();
};
static_assert(SearchCondition<TimeCondition>);

struct DepthCondition {
  explicit DepthCondition(Depth max_depth) : max_depth_(max_depth) {}
  bool ShouldContinueIteration() const { return cur_depth < max_depth_; }

  bool IsTimeToExit() const { return false; }

  void Update(const IterationInfo& info) { cur_depth = info.depth; }

  Depth cur_depth = 0;
  Depth max_depth_;
};
static_assert(SearchCondition<DepthCondition>);

using Condition = std::variant<TimeCondition, DepthCondition>;

struct Pondering {
  bool ShouldContinueIteration() const { return !IsTimeToExit(); }

  bool IsTimeToExit() const {
    if (pondermiss) return true;
    if (!condition) return false;
    return std::visit(
        [](const auto& unwrapped_control) -> bool {
          return unwrapped_control.IsTimeToExit();
        },
        *condition);
  }
  void Update(const IterationInfo& info) {
    if (!condition) return;
    std::visit(
        [&info](auto& unwrapped_condition) {
          unwrapped_condition.Update(info);
        },
        *condition);
  }

  std::optional<Condition> condition;

  bool pondermiss = false;
};
static_assert(SearchCondition<Pondering>);

/**
 * \brief Class that represents a chess engine.
 *
 * \author nook0110
 */
class ChessEngine {
 public:
  explicit ChessEngine(Position position = PositionFactory{}(),
                       std::ostream& o_stream = std::cout)
      : o_stream_(o_stream) {
    SetPosition(std::move(position));
  }

  void SetPosition(Position position) {
    position_ = position;
    searcher_.SetPosition(std::move(position));
  }

  void GoPonder(Pondering& conditions) {
    position_.DoMove(best_move_);
    position_.DoMove(*ponder_move_);
    searcher_.SetPosition(position_);

    ComputeBestMove(conditions);
  }

  void ComputeBestMove(SearchCondition auto& conditions);

  [[nodiscard]] const Move& GetCurrentBestMove() const;

  void PrintBestMove() { o_stream_ << BestMoveInfo{GetCurrentBestMove()}; }

  void PrintBestMove(const BestMoveInfo& bm_info) { o_stream_ << bm_info; }

 private:
  class EBFsInfo {
    void Update(std::size_t searched_nodes) {
      if (previous_nodes_ != 0) {
        ebfs_.push_back(static_cast<float>(searched_nodes) /
                        static_cast<float>(previous_nodes_));

        odd_even_average_ = 0;
        if (ebfs_.size() > 1) {
          for (auto it = ebfs_.rbegin(); ebfs_.rend() - it > 1; it += 2) {
            odd_even_average_ += *it;
          }
          odd_even_average_ /= static_cast<float>(ebfs_.size()) / 2.f;
        }
      }
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
                 Depth current_depth,
                 std::chrono::duration<double> search_time) {
    PrintInfo(ScoreInfo{eval});
    PrincipalVariationInfo pv{current_depth, searcher_.GetPrincipalVariation(
                                                 current_depth, position_)};
    PrintInfo(pv);
    PrintInfo(NodesInfo{info.searched_nodes});
    PrintInfo(NodesInfo{info.quiescence_nodes});
    PrintInfo(NodePerSecondInfo{static_cast<std::size_t>(
        (info.searched_nodes + info.quiescence_nodes) / search_time.count())});
  }

  template <class Info>
  void PrintInfo(const Info& info);

  std::optional<Eval> MakeIteration(Depth depth,
                                    const StopSearchCondition auto& end);

  std::ostream& o_stream_;

  Searcher searcher_;
  Position position_;

  Move best_move_;
  std::optional<Move> ponder_move_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
inline void SimpleChessEngine::ChessEngine::ComputeBestMove(
    SearchCondition auto& condition) {
  const TimePoint start_time = std::chrono::system_clock::now();
  searcher_.InitStartOfSearch();

  Searcher::DebugInfo info;

  for (Depth current_depth = 1;
       condition.ShouldContinueIteration() && current_depth < kMaxSearchPly;
       ++current_depth) {
    PrintInfo(DepthInfo{current_depth});
    const auto eval_optional = MakeIteration(current_depth, condition);
    if (!eval_optional) {
      break;
    }
    condition.Update(IterationInfo{searcher_, *eval_optional, current_depth});

    info += searcher_.GetInfo();
    PrintInfo(info, *eval_optional, current_depth,
              std::chrono::duration<double>{std::chrono::system_clock::now() -
                                            start_time});

    if (auto two_move_pv = searcher_.GetPrincipalVariation(2, position_);
        two_move_pv.size() > 1) {
      ponder_move_ = two_move_pv[1];
    }
    best_move_ = searcher_.GetCurrentBestMove();
  }

  PrintBestMove(BestMoveInfo{best_move_, ponder_move_});
}

inline const Move& ChessEngine::GetCurrentBestMove() const {
  return searcher_.GetCurrentBestMove();
}

inline std::optional<Eval> ChessEngine::MakeIteration(
    const Depth current_depth, const StopSearchCondition auto& condition) {
  constexpr auto neg_inf = std::numeric_limits<Eval>::min() / 2;
  constexpr auto pos_inf = std::numeric_limits<Eval>::max() / 2;

  return searcher_.Search<true>(condition, current_depth, current_depth,
                                neg_inf, pos_inf);
}

template <class Info>
void ChessEngine::PrintInfo(const Info& info) {
  o_stream_ << info;
}

inline std::ostream& operator<<(std::ostream& out,
                                const DepthInfo& depth_info) {
  return out << "info depth " << static_cast<size_t>(depth_info.current_depth)
             << std::endl;
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
  out << "bestmove " << bm_info.move;
  if (bm_info.ponder) {
    out << " ponder " << *bm_info.ponder;
  }
  return out << std::endl;
}

inline std::ostream& operator<<(std::ostream& out, const EBFInfo& ebf_info) {
  out << "info last ebf " << ebf_info.last_ebf << std::endl;
  out << "info avg odd-even-ebf " << ebf_info.avg_odd_even_ebf << std::endl;
  out << "info avg ebf " << ebf_info.avg_ebf << std::endl;
  return out;
}
}  // namespace SimpleChessEngine
