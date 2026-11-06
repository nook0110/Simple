#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <optional>

#include "Evaluation.h"
#include "ExitCondition.h"
#include "Move.h"
#include "NodeType.h"
#include "Searcher.h"
#include "Settings.h"
#include "Utility.h"

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
  using SharedTranspositionTable =
      std::shared_ptr<Searcher::SearcherTranspositionTable>;
  using SharedTablebase = std::shared_ptr<const Tablebase::Syzygy>;

  explicit ChessEngine(
      Position position = PositionFactory{}(),
      std::ostream& o_stream = std::cout,
      SharedTranspositionTable transposition_table =
          std::make_shared<Searcher::SearcherTranspositionTable>(),
      bool reporting = true, Depth initial_depth = 1,
      SharedTablebase tablebase = {})
      : o_stream_(o_stream),
        searcher_(position, std::move(transposition_table), std::move(tablebase)),
        reporting_(reporting),
        initial_depth_(initial_depth) {
    SetPosition(std::move(position));
  }

  void SetPosition(Position position) {
    position_ = position;
    searcher_.SetPosition(std::move(position));
  }

  void SetTablebase(SharedTablebase tablebase) {
    searcher_.SetTablebase(std::move(tablebase));
  }

  void GoPonder(Pondering& conditions) { ComputeBestMove(conditions); }
  void NewGame() {
    previous_score_.reset();
    previous_time_reduction_ = kInitialTimeReduction;
  }

  void ComputeBestMove(SearchCondition auto& conditions);

  [[nodiscard]] const Move& GetCurrentBestMove() const;

  void PrintBestMove() {
    if (!reporting_) return;
    o_stream_ << BestMoveInfo{GetCurrentBestMove(), std::nullopt};
  }

  void PrintBestMove(const BestMoveInfo& bm_info) {
    if (reporting_) o_stream_ << bm_info;
  }

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
                       : 0};
    }

   private:
    float odd_even_average_;
    std::vector<float> ebfs_;
    size_t previous_nodes_ = 0;
  };

  void PrintInfo(const DebugInfo& info, Eval eval, Depth current_depth,
                 std::chrono::duration<double> search_time) {
    if (!reporting_) return;
    PrintInfo(ScoreInfo{eval});
    PrincipalVariationInfo pv{current_depth, searcher_.GetPrincipalVariation(
                                                 current_depth, position_)};
    PrintInfo(pv);
    PrintInfo(NodesInfo{info.searched_nodes});
    PrintInfo(NodePerSecondInfo{static_cast<std::size_t>(
        (info.searched_nodes + info.quiescence_nodes) / search_time.count())});
    for (const auto& [name, counter] : kDebugCounters) {
      if (counter != &DebugInfo::searched_nodes) {
        o_stream_ << "info " << name << " " << info.*counter << "\n";
      }
    }
  }

  template <class Info>
  void PrintInfo(const Info& info);

  struct Window {
    static constexpr auto neg_inf = std::numeric_limits<Eval>::min() / 2;
    static constexpr auto pos_inf = std::numeric_limits<Eval>::max() / 2;

    Eval GetLowerBound() const {
      if constexpr (Settings::SearchParameters::AspirationWindowSettings::
                        kEnabled) {
        return lower_bound;
      }
      return neg_inf;
    }
    Eval GetUpperBound() const {
      if constexpr (Settings::SearchParameters::AspirationWindowSettings::
                        kEnabled) {
        return upper_bound;
      }
      return pos_inf;
    }

    void FailedUpper() {
      upper_bound +=
          Settings::SearchParameters::AspirationWindowSettings::kDelta / 3;
    }
    void FailedLower() {
      lower_bound -=
          Settings::SearchParameters::AspirationWindowSettings::kDelta / 3;
    }

    void SetNewEval(Eval eval) {
      lower_bound =
          eval - Settings::SearchParameters::AspirationWindowSettings::kDelta;
      upper_bound =
          eval + Settings::SearchParameters::AspirationWindowSettings::kDelta;
    }

   private:
    Eval lower_bound = neg_inf;
    Eval upper_bound = pos_inf;
  };

  std::optional<Eval> MakeIteration(Window window, Depth depth,
                                    const StopSearchCondition auto& end);

  static double Interpolate(double x, double x0, double x1, double y0,
                            double y1) {
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
  }

  static constexpr double kInitialTimeReduction = 0.85;
  static constexpr double kStockfishUnitsPerEval =
      208.0 / Settings::SearchParameters::AspirationWindowSettings::kPawnValue;

  std::ostream& o_stream_;

  Searcher searcher_;
  Position position_;

  Move best_move_{Move::None()};
  std::optional<Move> ponder_move_;
  bool reporting_;
  Depth initial_depth_;
  std::optional<Eval> previous_score_;
  double previous_time_reduction_ = kInitialTimeReduction;
};
}  // namespace SimpleChessEngine

namespace SimpleChessEngine {
inline void SimpleChessEngine::ChessEngine::ComputeBestMove(
    SearchCondition auto& condition) {
  const TimePoint start_time = std::chrono::system_clock::now();
  searcher_.InitStartOfSearch();

  const auto legal_moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position_);
  if (legal_moves.empty()) {
    PrintBestMove(BestMoveInfo{Move::None(), std::nullopt});
    return;
  }
  best_move_ = legal_moves.front();
  ponder_move_.reset();

  if (const auto& tablebase = searcher_.GetTablebase(); tablebase) {
    if (const auto tablebase_move = tablebase->RootMove(position_)) {
      best_move_ = *tablebase_move;
      PrintBestMove(BestMoveInfo{best_move_, std::nullopt});
      return;
    }
  }

  DebugInfo info;
  Eval eval = position_.Evaluate();
  EBFsInfo ebfs;
  Window window{};
  Depth current_depth = initial_depth_;
  double total_best_move_changes = 0;
  Depth last_best_move_depth = 0;
  Move last_best_move = Move::None();
  double time_reduction = 1;
  std::array<Eval, 4> iteration_values{};
  iteration_values.fill(previous_score_.value_or(0));
  std::size_t iteration_index = 0;

  for (current_depth = initial_depth_;
       condition.ShouldContinueIteration() && current_depth < kMaxSearchPly;
       ++current_depth) {
    PrintInfo(DepthInfo{current_depth});
    const auto eval_optional = MakeIteration(window, current_depth, condition);
    if (!eval_optional) {
      break;
    }
    eval = *eval_optional;
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

    ebfs.Update(searcher_.GetInfo().searched_nodes);
    info += searcher_.GetInfo();

    const Move& iteration_best_move = searcher_.GetCurrentBestMove();
    if (iteration_best_move != last_best_move) {
      last_best_move = iteration_best_move;
      last_best_move_depth = current_depth;
    }
    total_best_move_changes = total_best_move_changes / 2 +
                              static_cast<double>(
                                  searcher_.TakeBestMoveChanges());
    const double total_nodes = static_cast<double>(std::max<std::size_t>(
        1, info.searched_nodes + info.quiescence_nodes));
    const double nodes_effort =
        static_cast<double>(searcher_.GetRootEffort(iteration_best_move)) *
        100000.0 / total_nodes;
    double falling_eval = 1.728;
    if (previous_score_) {
      falling_eval =
          (11.48 +
           2.30 * kStockfishUnitsPerEval * (*previous_score_ - eval) +
           1.1 * kStockfishUnitsPerEval *
               (iteration_values[iteration_index] - eval)) /
          100.0;
    }
    falling_eval = std::clamp(falling_eval, 0.576, 1.728);
    time_reduction = std::clamp(
        Interpolate(static_cast<double>(current_depth - last_best_move_depth),
                    4.96, 18.79, 0.639, 1.712),
        0.629, 1.544);
    const double reduction =
        (1.468 + previous_time_reduction_) / (2.284 * time_reduction);
    const double best_move_instability =
        1.077 + 2.229 * total_best_move_changes;
    const double high_best_move_effort = std::clamp(
        Interpolate(nodes_effort, 75800.0, 104510.0, 0.969, 0.714), 0.693,
        0.838);
    double time_scale =
        falling_eval * reduction * best_move_instability * high_best_move_effort;
    if (legal_moves.size() == 1) {
      time_scale = std::min(time_scale, 1.0);
    }
    const bool proven_result =
        eval >= -kMateValue - 3 || eval == kMateValue + 2;
    condition.Update(IterationInfo{searcher_, eval, current_depth, time_scale,
                                   proven_result});
    iteration_values[iteration_index] = eval;
    iteration_index = (iteration_index + 1) % iteration_values.size();
    PrintInfo(info, eval, current_depth,
              std::chrono::duration<double>{std::chrono::system_clock::now() -
                                            start_time});
    PrintInfo(ebfs.GetInfo());
    if (reporting_) {
      o_stream_ << "info time "
                << std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - start_time)
                       .count()
                << "\n";
    }

    if (std::ranges::find(legal_moves, searcher_.GetCurrentBestMove()) !=
        legal_moves.end()) {
      best_move_ = searcher_.GetCurrentBestMove();
    }
    ponder_move_.reset();
    Position ponder_position = position_;
    const auto legal_root_moves =
        MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(
            ponder_position);
    if (std::ranges::find(legal_root_moves, best_move_) !=
        legal_root_moves.end()) {
      ponder_position.DoMove(best_move_);
      if (auto ponder_pv =
              searcher_.GetPrincipalVariation(1, std::move(ponder_position));
          !ponder_pv.empty()) {
        ponder_move_ = ponder_pv.front();
      }
    }
    window.SetNewEval(eval);
  }

  PrintInfo(info, eval, current_depth,
            std::chrono::duration<double>{std::chrono::system_clock::now() -
                                          start_time});

  previous_score_ = eval;
  previous_time_reduction_ = time_reduction;
  PrintBestMove(BestMoveInfo{best_move_, ponder_move_});
}

inline const Move& ChessEngine::GetCurrentBestMove() const {
  return best_move_;
}

inline std::optional<Eval> ChessEngine::MakeIteration(
    const Window window, const Depth current_depth,
    const StopSearchCondition auto& condition) {
  if (reporting_) {
    o_stream_ << "info window " << window.GetLowerBound() << " "
              << window.GetUpperBound() << "\n";
  }
  return searcher_.Search<NodeType::kPV>(condition, current_depth,
                                         current_depth, window.GetLowerBound(),
                                         window.GetUpperBound());
}

template <class Info>
void ChessEngine::PrintInfo(const Info& info) {
  if (reporting_) o_stream_ << info;
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
