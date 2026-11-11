#pragma once

#include <cassert>

#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine {
template <class T>
concept ReversibleMove = requires(Position &position, const T &move,
                                  const Position::IrreversibleData &undo_data) {
  position.DoMove(move);
  position.UndoMove(move, undo_data);
};

template <ReversibleMove PlayedMove>
class ScopedMove {
 public:
  [[nodiscard]] ScopedMove(Position &position, const PlayedMove &move,
                           const Position::IrreversibleData &undo_data)
      : position_(position), move_(move), undo_data_(undo_data) {
    assert(undo_data_ == position_.GetIrreversibleData());
    position_.DoMove(move_);
  }

  ~ScopedMove() { position_.UndoMove(move_, undo_data_); }

  ScopedMove(const ScopedMove &) = delete;
  ScopedMove(ScopedMove &&) = delete;
  ScopedMove &operator=(const ScopedMove &) = delete;
  ScopedMove &operator=(ScopedMove &&) = delete;

 private:
  Position &position_;
  const PlayedMove move_;
  const Position::IrreversibleData &undo_data_;
};
}  // namespace SimpleChessEngine
