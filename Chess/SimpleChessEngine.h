#pragma once
#include <cassert>
#include <chrono>
#include <cmath>
#include <numeric>
#include <variant>

#include "Evaluation.h"
#include "ExitCondition.h"
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
   public:
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
      previous_nodes_ = searched_nodes;
    }

    EBFInfo GetInfo() const {
      return EBFInfo{
          ebfs_.empty() ? 0 : ebfs_.back(), odd_even_average_,
          ebfs_.size() ? std::reduce(ebfs_.begin(), ebfs_.end()) / ebfs_.size()
                       : INFINITY};
    }

   private:
    float odd_even_average_;
    std::vector<float> ebfs_;
    size_t previous_nodes_ = 0;
  };

  void PrintInfo(const DebugInfo& info, Eval eval, Depth current_depth,
                 std::chrono::duration<double> search_time) {
    PrintInfo(ScoreInfo{eval});
    PrincipalVariationInfo pv{current_depth, searcher_.GetPrincipalVariation(
                                                 current_depth, position_)};
    PrintInfo(pv);
    PrintInfo(NodesInfo{info.searched_nodes});
    PrintInfo(NodePerSecondInfo{static_cast<std::size_t>(
        (info.searched_nodes + info.quiescence_nodes) / search_time.count())});
    o_stream_ << "info quiescence_nodes " << info.quiescence_nodes << "\n";
    o_stream_ << "info zws_researches " << info.zws_researches << "\n";
    o_stream_ << "info tt_hits " << info.tt_hits << "\n";
    o_stream_ << "info tt_cuts " << info.tt_cuts << "\n";
    o_stream_ << "info rfp_cuts " << info.rfp_cuts << "\n";
    o_stream_ << "info nmp_tries " << info.nmp_tries << "\n";
    o_stream_ << "info nmp_cuts " << info.nmp_cuts << "\n";
  }

  template <class Info>
  void PrintInfo(const Info& info);

  struct Window {
    static constexpr auto neg_inf = std::numeric_limits<Eval>::min() / 2;
    static constexpr auto pos_inf = std::numeric_limits<Eval>::max() / 2;

    Eval GetLowerBound() const { return neg_inf; }
    Eval GetUpperBound() const { return pos_inf; }

    void FailedUpper() { upper_coeff *= 2; }
    void FailedLower() { lower_coeff *= 2; }

    void SetNewEval(Eval eval) {
      lower_bound = eval;
      upper_bound = eval;
      lower_coeff = 1;
      upper_coeff = 1;
    }

   private:
    Eval lower_bound = neg_inf;
    Eval upper_bound = pos_inf;

    size_t lower_coeff = 1;
    size_t upper_coeff = 1;
    static constexpr Eval kPawnValue =
        kPieceValues[static_cast<size_t>(Piece::kPawn)]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)];
    static constexpr Eval kDelta = kPawnValue / 8;
  };

  std::optional<Eval> MakeIteration(Window window, Depth depth,
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

  DebugInfo info;
  EBFsInfo ebfs;
  Window window{};

  for (Depth current_depth = 1;
       condition.ShouldContinueIteration() && current_depth < kMaxSearchPly;
       ++current_depth) {
    PrintInfo(DepthInfo{current_depth});
    const auto eval_optional = MakeIteration(window, current_depth, condition);
    if (!eval_optional) {
      break;
    }
    const auto eval = *eval_optional;
    if (eval >= window.GetUpperBound()) {
      window.FailedUpper();
      --current_depth;
      continue;
    }
    if (eval <= window.GetLowerBound()) {
      window.FailedLower();
      --current_depth;
      continue;
    }

    condition.Update(IterationInfo{searcher_, eval, current_depth});
    ebfs.Update(searcher_.GetInfo().searched_nodes);
    info += searcher_.GetInfo();
    PrintInfo(info, eval, current_depth,
              std::chrono::duration<double>{std::chrono::system_clock::now() -
                                            start_time});
    PrintInfo(ebfs.GetInfo());
    o_stream_ << "info time "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now() - start_time)
                     .count()
              << "\n";

    if (auto two_move_pv = searcher_.GetPrincipalVariation(2, position_);
        two_move_pv.size() > 1) {
      ponder_move_ = two_move_pv[1];
    }
    best_move_ = searcher_.GetCurrentBestMove();
    window.SetNewEval(eval);
  }

  PrintBestMove(BestMoveInfo{best_move_, ponder_move_});
}

inline const Move& ChessEngine::GetCurrentBestMove() const {
  return searcher_.GetCurrentBestMove();
}

inline std::optional<Eval> ChessEngine::MakeIteration(
    const Window window, const Depth current_depth,
    const StopSearchCondition auto& condition) {
  o_stream_ << "info window " << window.GetLowerBound() << " "
            << window.GetUpperBound() << "\n";
  return searcher_.Search<true>(condition, current_depth, current_depth,
                                window.GetLowerBound(), window.GetUpperBound());
}

template <class Info>
void ChessEngine::PrintInfo(const Info& info) {
  o_stream_ << info;
}

inline std::ostream& operator<<(std::ostream& out,
                                const DepthInfo& depth_info) {
  return out << "info depth " << +depth_info.current_depth << std::endl;
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
