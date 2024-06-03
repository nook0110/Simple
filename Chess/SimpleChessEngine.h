#pragma once
#include <cassert>
#include <chrono>
#include <variant>

#include "Evaluation.h"
#include "Move.h"
#include "Searcher.h"

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
  std::vector<Move> best_moves;
};

struct BestMoveInfo {
  const Move& move;
};

struct PrincipalVariationHitsInfo {
  std::size_t pv_hits{};
};

class InfoPrinter {
 public:
  virtual ~InfoPrinter() = default;
  virtual void operator()(const DepthInfo& depth_info) const {}
  virtual void operator()(const ScoreInfo& score_info) const {}
  virtual void operator()(const NodesInfo& nodes_info) const {}
  virtual void operator()(const NodePerSecondInfo& nps_info) const {}
  virtual void operator()(
      const PrincipalVariationInfo& principal_variation) const {}
  virtual void operator()(
      const PrincipalVariationHitsInfo& pv_hits_info) const {}
  virtual void operator()(const BestMoveInfo& best_move) const {}
};

/**
 * \brief Class that represents a chess engine.
 *
 * \author nook0110
 */
class ChessEngine {
 public:
  struct SearchTimeInfo {
    std::array<size_t, 2> player_time;

    std::array<size_t, 2> player_inc;
  };

  explicit ChessEngine(
      const Position position = PositionFactory{}(),
      std::unique_ptr<InfoPrinter> printer = std::make_unique<InfoPrinter>())
      : printer_(std::move(printer)), searcher_(position) {}

  void SetInfoPrinter(std::unique_ptr<InfoPrinter> printer) {
    printer_ = std::move(printer);
  }

  void SetPosition(const Position position) { searcher_.SetPosition(position); }

  void ComputeBestMove(size_t depth);

  void ComputeBestMove(const std::chrono::milliseconds left_time,
                       const std::chrono::milliseconds inc_time);

  [[nodiscard]] const Move& GetCurrentBestMove() const;

  void PrintBestMove() const {
    printer_->operator()(BestMoveInfo{GetCurrentBestMove()});
  }

  void PrintBestMove(const Move& move) const {
    printer_->operator()(BestMoveInfo{move});
  }

 private:
  template <class Info>
  void PrintInfo(const Info& info);

  std::unique_ptr<InfoPrinter> printer_;

  Searcher searcher_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
inline void ChessEngine::ComputeBestMove(const size_t depth) {
  assert(false);
}

inline void ChessEngine::ComputeBestMove(
    const std::chrono::milliseconds left_time,
    const std::chrono::milliseconds inc_time = {}) {
  const auto start_time = std::chrono::high_resolution_clock::now();
  constexpr size_t kAverageGameLength = 50;

  const auto time_for_move = left_time / kAverageGameLength + inc_time;
  constexpr auto kTimeRatio = 1.1;
  static constexpr size_t max_last_best_move_change = 10;
  constexpr auto min_inf = std::numeric_limits<Eval>::min() / 2;
  constexpr auto plus_inf = std::numeric_limits<Eval>::max() / 2;

  Searcher::DebugInfo info{};

  searcher_.InitStartOfSearch();

  Move previous_best_move{};
  size_t last_best_move_change{};
  for (size_t current_depth = 1;
       time_for_move >
           (std::chrono::high_resolution_clock::now() - start_time) *
               kTimeRatio  // check if we have time for another iteration
       &&
       last_best_move_change < max_last_best_move_change  // check if best move
                                                          // changed recently
       ;) {
    PrintInfo(DepthInfo{current_depth});

    const auto eval_optional =
        searcher_.Search<true>(time_for_move + start_time, current_depth,
                               current_depth, min_inf, plus_inf);

    if (!eval_optional) {
      PrintBestMove(previous_best_move);
      return;
    }

    const auto& eval = *eval_optional;

    info += searcher_.GetInfo();

    PrintInfo(ScoreInfo{eval});

    // check if best move changed
    if (previous_best_move == searcher_.GetCurrentBestMove()) {
      // increase last change
      ++last_best_move_change;
    } else {
      // reset last change
      last_best_move_change = 0;
    }

    previous_best_move = GetCurrentBestMove();
    PrincipalVariationInfo pv;
    for (size_t ply = 0; ply < current_depth; ++ply) {
      if (!searcher_.GetPV().CheckPV(ply)) break;
      pv.best_moves.push_back(searcher_.GetPV().GetPV(ply));
    }
    PrintInfo(pv);
    PrintInfo(NodesInfo{info.searched_nodes});
    PrintInfo(NodesInfo{info.quiescence_nodes});
    PrintInfo(NodePerSecondInfo{static_cast<std::size_t>(
        (info.searched_nodes + info.quiescence_nodes) /
        (std::chrono::duration<double>{
             std::chrono::high_resolution_clock::now() - start_time})
            .count())});
    PrintInfo(PrincipalVariationHitsInfo{info.pv_hits});
    info = Searcher::DebugInfo{};

    // increase the depth
    current_depth++;
  }

  PrintBestMove(previous_best_move);
}

inline const Move& ChessEngine::GetCurrentBestMove() const {
  return searcher_.GetCurrentBestMove();
}

template <class Info>
void ChessEngine::PrintInfo(const Info& info) {
  (*printer_)(info);
}
}  // namespace SimpleChessEngine
