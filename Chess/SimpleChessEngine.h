#pragma once
#include <chrono>
#include <variant>
#include <vector>

#include "Evaluation.h"
#include "Move.h"
#include "Searcher.h"

namespace SimpleChessEngine
{
struct DepthInfo
{
  size_t current_depth = 0;
};

struct ScoreInfo
{
  Eval current_eval = Eval{};
};

struct NodePerSecondInfo
{
  size_t nodes_per_second = 0;
};

struct PrincipalVariation
{
  const Move& best_move;
};

struct BestMove
{
  const Move& move;
};

class InfoPrinter
{
 public:
  virtual ~InfoPrinter() = default;
  virtual void operator()(const DepthInfo& depth_info) const {}
  virtual void operator()(const ScoreInfo& score_info) const {}
  virtual void operator()(const NodePerSecondInfo& nps_info) const {}
  virtual void operator()(const PrincipalVariation& principal_variation) const
  {}
  virtual void operator()(const BestMove& best_move) const {}
};

/**
 * \brief Class that represents a chess engine.
 *
 * \author nook0110
 */
class ChessEngine
{
 public:
  struct SearchTimeInfo
  {
    std::array<size_t, 2> player_time;

    std::array<size_t, 2> player_inc;
  };

  explicit ChessEngine(
      const Position position = PositionFactory{}(),
      std::unique_ptr<InfoPrinter> printer = std::make_unique<InfoPrinter>())
      : printer_(std::move(printer)), searcher_(position)
  {}

  void SetInfoPrinter(std::unique_ptr<InfoPrinter> printer)
  {
    printer_ = std::move(printer);
  }

  void SetPosition(const Position position) { searcher_.SetPosition(position); }

  void ComputeBestMove(size_t depth);

  void ComputeBestMove(std::chrono::milliseconds left_time);

  [[nodiscard]] const Move& GetCurrentBestMove() const;

  [[nodiscard]] const TranspositionTable& GetTranspositionTable() const;

  void PrintBestMove() const
  {
    printer_->operator()(BestMove{GetCurrentBestMove()});
  }

  void PrintBestMove(const Move& move) const
  {
    printer_->operator()(BestMove{move});
  }

 private:
  template <class Info>
  void PrintInfo(const Info& info);

  std::unique_ptr<InfoPrinter> printer_;

  Searcher searcher_;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine
{
inline void ChessEngine::ComputeBestMove(const size_t depth)
{
  auto alpha = std::numeric_limits<Eval>::min() / 2;
  auto beta = std::numeric_limits<Eval>::max() / 2;

  PrintInfo(ScoreInfo{searcher_.Search<true>(0, alpha, beta)});

  for (size_t current_depth = 1; current_depth < depth;)
  {
    PrintInfo(DepthInfo{current_depth});

    static constexpr auto window_size = 100;
    const auto eval = searcher_.Search<true>(current_depth, alpha, beta);

    // check if true eval is out of window
    if (eval <= alpha)
    {
      // search again with a wider window
      alpha = eval - window_size;

      continue;
    }

    // check if true eval is out of window
    if (eval >= beta)
    {
      // search again with a wider window
      beta = eval + window_size;
      continue;
    }

    // set the window
    alpha = eval - window_size;
    beta = eval + window_size;

    // increase the depth
    current_depth++;

    PrintInfo(ScoreInfo{eval});
  }

  PrintBestMove();
}

inline void ChessEngine::ComputeBestMove(
    const std::chrono::milliseconds left_time)
{
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto time_for_move = left_time / 3;
  constexpr auto kTimeRatio = 45;
  static constexpr size_t max_last_best_move_change = 4;

  auto alpha = std::numeric_limits<Eval>::min();
  auto beta = std::numeric_limits<Eval>::max();

  Move previous_best_move{};
  size_t last_best_move_change{};
  for (size_t current_depth = 1;
       time_for_move >
           (std::chrono::high_resolution_clock::now() - start_time) *
               kTimeRatio  // check if we have time for another iteration
       &&
       last_best_move_change < max_last_best_move_change  // check if best move
                                                          // changed recently
       ;)
  {
    PrintInfo(DepthInfo{current_depth});

    static constexpr auto window_size = 200;
    const auto eval = searcher_.Search<true>(current_depth, alpha, beta);

    PrintInfo(ScoreInfo{eval});

    // check if true eval is out of window
    if (eval < alpha)
    {
      // search again with a wider window
      alpha = eval - window_size;

      continue;
    }

    // check if true eval is out of window
    if (eval > beta)
    {
      // search again with a wider window
      beta = eval + window_size;
      continue;
    }

    // set the window
    alpha = eval - window_size;
    beta = eval + window_size;

    // increase the depth
    current_depth++;

    // check if best move changed
    if (previous_best_move == searcher_.GetCurrentBestMove())
    {
      // increase last change
      ++last_best_move_change;
    }
    else
    {
      // reset last change
      last_best_move_change = 0;
    }

    previous_best_move = GetCurrentBestMove();

    PrintInfo(PrincipalVariation(previous_best_move));
  }

  PrintBestMove(previous_best_move);
}

inline const Move& ChessEngine::GetCurrentBestMove() const
{
  return searcher_.GetCurrentBestMove();
}

inline const TranspositionTable& ChessEngine::GetTranspositionTable() const
{
  return searcher_.GetTranspositionTable();
}

template <class Info>
void ChessEngine::PrintInfo(const Info& info)
{
  (*printer_)(info);
}
}  // namespace SimpleChessEngine
