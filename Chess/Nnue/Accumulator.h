#pragma once

#include <cstdint>
#include <vector>

#include "Stockfish/nnue/nnue_accumulator.h"

namespace SimpleChessEngine::Nnue {
struct CachedOutput {
  bool valid = false;
  int psqt = 0;
  int positional = 0;
};

class AccumulatorStack {
 public:
  AccumulatorStack() : outputs_(1) {}
  AccumulatorStack(const AccumulatorStack& other)
      : stack_(other.stack_), outputs_(1, other.outputs_[other.Size() - 1]),
        generation_(other.generation_) {}
  AccumulatorStack& operator=(const AccumulatorStack& other) {
    if (this != &other) {
      stack_ = other.stack_;
      outputs_.assign(1, other.outputs_[other.Size() - 1]);
      generation_ = other.generation_;
    }
    return *this;
  }

  Stockfish::Dirties& Push() {
    if (outputs_.size() <= Size()) outputs_.resize(Size() + 1);
    outputs_[Size()].valid = false;
    return stack_.push();
  }
  void Pop() {
    const bool underflow = Size() == 1;
    stack_.pop();
    if (underflow) Invalidate();
  }
  void Invalidate() {
    stack_.invalidate();
    for (auto& output : outputs_) output.valid = false;
  }
  void InvalidateOutput() { Output().valid = false; }
  void Synchronize(std::uint32_t generation) {
    if (generation_ == generation) return;
    generation_ = generation;
    stack_.reset();
    outputs_.resize(1);
    Invalidate();
  }
  std::size_t Size() const { return stack_.length(); }
  CachedOutput& Output() { return outputs_[Size() - 1]; }
  Stockfish::Eval::NNUE::AccumulatorStack& Native() { return stack_; }

 private:
  Stockfish::Eval::NNUE::AccumulatorStack stack_;
  std::vector<CachedOutput> outputs_;
  std::uint32_t generation_ = 0;
};
}  // namespace SimpleChessEngine::Nnue
