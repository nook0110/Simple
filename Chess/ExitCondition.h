#pragma once
#include <atomic>
#include <memory>
#include <variant>
#include "Evaluation.h"
#include "Searcher.h"
namespace SimpleChessEngine {
class Searcher;

using StopSignal = std::shared_ptr<std::atomic_bool>;

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
  explicit TimeCondition(std::chrono::milliseconds time_for_move,
                         StopSignal stop_signal = {})
      : time_for_move_(std::move(time_for_move)),
        stop_signal_(std::move(stop_signal)) {}

  bool ShouldContinueIteration() const { return !IsTimeToExit(); }

  bool IsTimeToExit() const {
    return (stop_signal_ && stop_signal_->load(std::memory_order_relaxed)) ||
           time_for_move_ < (std::chrono::system_clock::now() - start_time_);
  }

  void Update(const IterationInfo&) const {}

  std::chrono::milliseconds time_for_move_;
  TimePoint start_time_ = std::chrono::system_clock::now();
  StopSignal stop_signal_;
};
static_assert(SearchCondition<TimeCondition>);

struct DepthCondition {
  explicit DepthCondition(Depth max_depth, StopSignal stop_signal = {})
      : max_depth_(max_depth), stop_signal_(std::move(stop_signal)) {}
  bool ShouldContinueIteration() const { return cur_depth < max_depth_; }

  bool IsTimeToExit() const {
    return stop_signal_ && stop_signal_->load(std::memory_order_relaxed);
  }

  void Update(const IterationInfo& info) { cur_depth = info.depth; }

  Depth cur_depth = 0;
  Depth max_depth_;
  StopSignal stop_signal_;
};
static_assert(SearchCondition<DepthCondition>);

using Condition = std::variant<TimeCondition, DepthCondition>;

struct Pondering {
  explicit Pondering(StopSignal signal = {})
      : stop_signal(std::move(signal)) {}

  bool ShouldContinueIteration() const { return !IsTimeToExit(); }

  bool IsTimeToExit() const {
    if (stop_signal && stop_signal->load(std::memory_order_relaxed)) return true;
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

  StopSignal stop_signal;
};
static_assert(SearchCondition<Pondering>);
}  // namespace SimpleChessEngine
