#pragma once
#include <cassert>
#include <chrono>
#include <numeric>
#include <variant>

#include "Conditions.h"
#include "Evaluation.h"
#include "IterationInfo.h"
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
  Depth current_depth = 0;
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

  void GoPonder(Pondering& conditions) { ComputeBestMove(conditions); }

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

  struct Info {
    ScoreInfo score;
    PrincipalVariationInfo pv;
    NodesInfo searched_nodes;
    NodesInfo quiescence_nodes;
    NodePerSecondInfo nps;
    Searcher::DebugInfo searcher_info;

    void Update(const Searcher& searcher, Eval eval, Depth current_depth,
                const std::chrono::duration<double>& search_time) {
      searcher_info += searcher.GetInfo();
      score = {eval};
      pv = {current_depth, searcher.GetPrincipalVariation(
                               current_depth, searcher.GetPosition())};
      searched_nodes = {searcher_info.searched_nodes};
      quiescence_nodes = {searcher_info.quiescence_nodes};
      nps = {static_cast<std::size_t>(
          static_cast<double>(searcher_info.searched_nodes +
                              searcher_info.quiescence_nodes) /
          search_time.count())};
    }
  };

  void PrintInfo(const Info& info) {
    const auto& [score, pv, searched_nodes, quiescence_nodes, nps, _] = info;
    PrintInfo(score);
    PrintInfo(pv);
    PrintInfo(searched_nodes);
    PrintInfo(quiescence_nodes);
    PrintInfo(nps);
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

  Info info;

  for (Depth current_depth = 1;
       condition.ShouldContinueIteration() && current_depth < kMaxSearchPly;
       ++current_depth) {
    const auto eval_optional = MakeIteration(current_depth, condition);
    if (!eval_optional) {
      PrintInfo(info);
      break;
    }
    info.Update(searcher_, *eval_optional, current_depth,
                std::chrono::duration<double>{std::chrono::system_clock::now() -
                                              start_time});
    PrintInfo(info);

    condition.Update(IterationInfo{searcher_, *eval_optional, current_depth});

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

  return searcher_.Search(condition, current_depth, current_depth, neg_inf,
                          pos_inf);
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
