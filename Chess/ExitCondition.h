#pragma once
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
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

  bool IsTimeToExit() const {
    return IsStopped() || (managed_ && stop_now_) || Elapsed() >= hard_limit_;
  }

  void Update(const IterationInfo& info) {
    time_scale_ = info.time_scale;
    stop_now_ = info.stop_now;
  }

  void ResumeFromPonder(double time_scale, bool stop_now) {
    if (!managed_) return;
    time_scale_ = time_scale;
    const std::chrono::duration<double, std::milli> budget =
        std::chrono::duration<double, std::milli>(soft_limit_) * time_scale_;
    stop_now_ = stop_now || Elapsed() > budget;
  }

  std::chrono::milliseconds soft_limit_;
  std::chrono::milliseconds hard_limit_;
  TimePoint start_time_ = std::chrono::steady_clock::now();
  StopSignal stop_signal_;
  bool managed_ = false;
  double time_scale_ = 1.0;
  bool stop_now_ = false;

  static constexpr double kDeepenFraction = 0.5;

 private:
  bool IsStopped() const {
    return stop_signal_ && stop_signal_->load(std::memory_order_relaxed);
  }

  std::chrono::steady_clock::duration Elapsed() const {
    return std::chrono::steady_clock::now() - start_time_;
  }
};
static_assert(SearchCondition<TimeCondition>);

class NodeBudget {
 public:
  explicit NodeBudget(std::uint64_t limit) : limit_(limit) {}

  void Spend() { searched_.fetch_add(1, std::memory_order_relaxed); }
  [[nodiscard]] bool IsSpent() const {
    return searched_.load(std::memory_order_relaxed) >= limit_;
  }

 private:
  std::atomic<std::uint64_t> searched_{0};
  const std::uint64_t limit_;
};

struct DepthCondition {
  explicit DepthCondition(Depth max_depth, StopSignal stop_signal = {})
      : max_depth_(max_depth), stop_signal_(std::move(stop_signal)) {}
  bool ShouldContinueIteration() const {
    return !IsTimeToExit() && cur_depth < max_depth_;
  }

  bool IsTimeToExit() const {
    return stop_signal_ && stop_signal_->load(std::memory_order_relaxed);
  }

  void Update(const IterationInfo& info) { cur_depth = info.depth; }

  Depth cur_depth = 0;
  Depth max_depth_;
  StopSignal stop_signal_;
};
static_assert(SearchCondition<DepthCondition>);

class NodeCondition {
 public:
  NodeCondition(Depth max_depth, StopSignal stop_signal,
                std::shared_ptr<NodeBudget> node_budget)
      : depth_condition_(max_depth, std::move(stop_signal)),
        node_budget_(std::move(node_budget)) {
    assert(node_budget_);
  }

  bool ShouldContinueIteration() const {
    return !node_budget_->IsSpent() &&
           depth_condition_.ShouldContinueIteration();
  }

  [[gnu::always_inline]] bool IsTimeToExit() const {
    return depth_condition_.IsTimeToExit() || node_budget_->IsSpent();
  }

  [[gnu::always_inline]] void CountNode() const { node_budget_->Spend(); }

  void Update(const IterationInfo& info) { depth_condition_.Update(info); }

 private:
  DepthCondition depth_condition_;
  std::shared_ptr<NodeBudget> node_budget_;
};
static_assert(SearchCondition<NodeCondition>);
static_assert(CountsNodes<NodeCondition>);
static_assert(!CountsNodes<DepthCondition>);
static_assert(!CountsNodes<TimeCondition>);

using Condition = std::variant<TimeCondition, DepthCondition, NodeCondition>;

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
    if (!HasCondition()) {
      pending_time_scale.store(info.time_scale, std::memory_order_relaxed);
      pending_stop.store(info.stop_now, std::memory_order_relaxed);
      return;
    }
    std::visit(
        [&info](auto& unwrapped_condition) {
          unwrapped_condition.Update(info);
        },
        *condition);
  }

  void WaitForPonderEnd() const {
    while (!HasCondition() &&
           !(stop_signal && stop_signal->load(std::memory_order_relaxed))) {
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
  }

  void SetCondition(Condition new_condition) {
    if (auto* time_condition = std::get_if<TimeCondition>(&new_condition)) {
      time_condition->ResumeFromPonder(
          pending_time_scale.load(std::memory_order_relaxed),
          pending_stop.load(std::memory_order_relaxed));
    }
    condition = std::move(new_condition);
    has_condition.store(true, std::memory_order_release);
  }

  std::optional<Condition> condition;
  std::atomic_bool has_condition{false};
  std::atomic<double> pending_time_scale{1.0};
  std::atomic_bool pending_stop{false};

  StopSignal stop_signal;

 private:
  bool HasCondition() const {
    return has_condition.load(std::memory_order_acquire);
  }
};
static_assert(SearchCondition<Pondering>);
}  // namespace SimpleChessEngine
