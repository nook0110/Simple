#pragma once
#include <atomic>
#include <chrono>
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
  double time_scale = 1.0;
  bool stop_now = false;
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
      : soft_limit_(time_for_move), hard_limit_(time_for_move),
        stop_signal_(std::move(stop_signal)) {}

  TimeCondition(std::chrono::milliseconds soft_limit,
                std::chrono::milliseconds hard_limit,
                StopSignal stop_signal = {})
      : soft_limit_(soft_limit), hard_limit_(hard_limit),
        stop_signal_(std::move(stop_signal)), managed_(true) {}

  bool ShouldContinueIteration() const {
    if (IsStopped()) return false;
    if (!managed_) return Elapsed() < soft_limit_;
    const std::chrono::duration<double, std::milli> deepen_limit =
        std::chrono::duration<double, std::milli>(soft_limit_) * time_scale_ *
        kDeepenFraction;
    return !stop_now_ && Elapsed() <= deepen_limit && Elapsed() < hard_limit_;
  }

  bool IsTimeToExit() const { return IsStopped() || Elapsed() > hard_limit_; }

  void Update(const IterationInfo& info) {
    time_scale_ = info.time_scale;
    stop_now_ = info.stop_now;
  }

  std::chrono::milliseconds soft_limit_;
  std::chrono::milliseconds hard_limit_;
  TimePoint start_time_ = std::chrono::system_clock::now();
  StopSignal stop_signal_;
  bool managed_ = false;
  double time_scale_ = 1.0;
  bool stop_now_ = false;

  static constexpr double kDeepenFraction = 0.5;

 private:
  bool IsStopped() const {
    return stop_signal_ && stop_signal_->load(std::memory_order_relaxed);
  }

  std::chrono::system_clock::duration Elapsed() const {
    return std::chrono::system_clock::now() - start_time_;
  }
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

  bool ShouldContinueIteration() const {
    if (stop_signal && stop_signal->load(std::memory_order_relaxed)) {
      return false;
    }
    if (!HasCondition()) return true;
    return std::visit(
        [](const auto& unwrapped_control) -> bool {
          return unwrapped_control.ShouldContinueIteration();
        },
        *condition);
  }

  bool IsTimeToExit() const {
    if (stop_signal && stop_signal->load(std::memory_order_relaxed)) return true;
    if (!HasCondition()) return false;
    return std::visit(
        [](const auto& unwrapped_control) -> bool {
          return unwrapped_control.IsTimeToExit();
        },
        *condition);
  }
  void Update(const IterationInfo& info) {
    if (!HasCondition()) return;
    std::visit(
        [&info](auto& unwrapped_condition) {
          unwrapped_condition.Update(info);
        },
        *condition);
  }

  void SetCondition(Condition new_condition) {
    condition = std::move(new_condition);
    has_condition.store(true, std::memory_order_release);
  }

  std::optional<Condition> condition;
  std::atomic_bool has_condition{false};

  StopSignal stop_signal;

 private:
  bool HasCondition() const {
    return has_condition.load(std::memory_order_acquire);
  }
};
static_assert(SearchCondition<Pondering>);
}  // namespace SimpleChessEngine
