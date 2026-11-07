#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../BitBoard.h"
#include "../Piece.h"
#include "../Player.h"
#include "Common.h"

namespace SimpleChessEngine::Nnue {
constexpr BitIndex kNoSquare = -1;

struct DirtyPiece {
  void Add(const Piece moved, const Player owner, const BitIndex source,
           const BitIndex destination) {
    piece[count] = moved;
    color[count] = owner;
    from[count] = source;
    to[count] = destination;
    ++count;
  }

  int count = 0;
  std::array<Piece, 3> piece{};
  std::array<Player, 3> color{};
  std::array<BitIndex, 3> from{};
  std::array<BitIndex, 3> to{};
};

template <IndexType kSize>
struct alignas(kCacheLineSize) Accumulator {
  std::int16_t accumulation[2][kSize];
  std::int32_t psqt_accumulation[2][kPsqtBuckets];
  std::array<bool, 2> computed;
};

struct CachedOutput {
  bool valid = false;
  int psqt = 0;
  int positional = 0;
};

class AccumulatorStack {
 public:
  AccumulatorStack() : dirty_(kInitialCapacity), outputs_(kInitialCapacity) {}

  AccumulatorStack(const AccumulatorStack& other)
      : dirty_(kInitialCapacity), outputs_(kInitialCapacity) {
    CopyTop(other);
  }

  AccumulatorStack& operator=(const AccumulatorStack& other) {
    if (this != &other) {
      size_ = 1;
      CopyTop(other);
    }
    return *this;
  }

  DirtyPiece& Push() {
    if (dirty_.size() <= size_) {
      dirty_.resize(std::max(dirty_.size() * 2, size_ + 1));
      outputs_.resize(dirty_.size());
    }
    Reset(small_);
    Reset(big_);
    outputs_[size_].valid = false;
    DirtyPiece& dirty = dirty_[size_];
    dirty.count = 0;
    dirty.piece[0] = Piece::kNone;
    ++size_;
    return dirty;
  }

  void Pop() {
    if (size_ > 1) {
      --size_;
      return;
    }
    Invalidate();
  }

  void Invalidate() {
    if (!small_.empty()) small_[0].computed = {false, false};
    if (!big_.empty()) big_[0].computed = {false, false};
    outputs_[size_ - 1].valid = false;
  }

  void InvalidateOutput() { outputs_[size_ - 1].valid = false; }

  [[nodiscard]] std::size_t Size() const { return size_; }
  [[nodiscard]] const DirtyPiece& Dirty(const std::size_t index) const {
    return dirty_[index];
  }
  [[nodiscard]] CachedOutput& Output() { return outputs_[size_ - 1]; }

  void Synchronize(const std::uint32_t generation) {
    if (generation_ == generation) return;
    generation_ = generation;
    for (auto& accumulator : small_) accumulator.computed = {false, false};
    for (auto& accumulator : big_) accumulator.computed = {false, false};
    for (std::size_t i = 0; i < size_; ++i) outputs_[i].valid = false;
  }

  template <IndexType kSize>
  std::vector<Accumulator<kSize>>& Accumulators() {
    auto& accumulators = Storage<kSize>();
    if (accumulators.size() < size_) {
      accumulators.resize(std::max(size_, kInitialCapacity));
    }
    return accumulators;
  }

 private:
  static constexpr std::size_t kInitialCapacity = 128;

  template <IndexType kSize>
  std::vector<Accumulator<kSize>>& Storage() {
    if constexpr (kSize == kSmallDimensions) {
      return small_;
    } else {
      static_assert(kSize == kBigDimensions);
      return big_;
    }
  }

  template <typename Vector>
  void Reset(Vector& accumulators) {
    if (accumulators.empty()) return;
    if (accumulators.size() <= size_) {
      accumulators.resize(std::max(accumulators.size() * 2, size_ + 1));
    }
    accumulators[size_].computed = {false, false};
  }

  template <typename Vector>
  static void CopyTopAccumulator(Vector& target, const Vector& source,
                                 const std::size_t top) {
    target.clear();
    if (source.size() > top) {
      target.reserve(kInitialCapacity);
      target.push_back(source[top]);
    }
  }

  void CopyTop(const AccumulatorStack& other) {
    const std::size_t top = other.size_ - 1;
    generation_ = other.generation_;
    outputs_[0] = other.outputs_[top];
    dirty_[0] = DirtyPiece{};
    CopyTopAccumulator(small_, other.small_, top);
    CopyTopAccumulator(big_, other.big_, top);
  }

  std::vector<DirtyPiece> dirty_;
  std::vector<CachedOutput> outputs_;
  std::vector<Accumulator<kSmallDimensions>> small_;
  std::vector<Accumulator<kBigDimensions>> big_;
  std::size_t size_ = 1;
  std::uint32_t generation_ = 0;
};
}  // namespace SimpleChessEngine::Nnue
