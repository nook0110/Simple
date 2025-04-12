#pragma once
#include "Chess/Searcher.h"
#include "Evaluation.h"
namespace SimpleChessEngine {
class Searcher;
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
}  // namespace SimpleChessEngine
