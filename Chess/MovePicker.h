#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Evaluation.h"
#include "Move.h"
#include "Piece.h"
#include "Utility.h"

namespace SimpleChessEngine {
class Searcher;

class MovePicker {
 public:
  using Moves = std::vector<Move>;
  enum class Stage : std::uint8_t {
    kGoodCaptures,
    kKillers,
    kQuiet,
    kBadCaptures,
    kEnd
  };

  MovePicker();
  MovePicker(const Move&) = delete;
  MovePicker(Move&&) = delete;
  void InitPicker(Moves&& moves, const Searcher& searcher);
  ~MovePicker() = default;

  MovePicker& operator=(MovePicker&&) = delete;
  MovePicker& operator=(const MovePicker&) = delete;

  Moves::const_iterator SelectNextMove(const Searcher& searcher,
                                       const Depth ply);

  void SkipMove(const Move& move);

  [[nodiscard]] bool Done() const;

  [[nodiscard]] Stage GetCurrentStage() const;

  [[nodiscard]] Moves::const_iterator begin() const { return moves_.begin(); }
  [[nodiscard]] Moves::const_iterator begin_quiet() const { return begin_quiet_; }
  [[nodiscard]] Moves::const_iterator current() const { return current_move_; }
  [[nodiscard]] Moves::const_iterator end() const { return moves_.end(); }

 private:
  static constexpr Eval kGoodCaptureThreshold = -50;

  Moves moves_;
  Moves::iterator begin_quiet_;
  Moves::iterator current_move_;

  struct MoveData {
    BitIndex from = {-1};
    BitIndex to = {};
    Piece captured = {};
    bool is_good_capture = false;
  };

  std::vector<MoveData> data_;
  std::vector<std::int64_t> history_;

  void Swap(size_t lhs, size_t rhs) {
    std::swap(moves_[lhs], moves_[rhs]);
    std::swap(data_[lhs], data_[rhs]);
    std::swap(history_[lhs], history_[rhs]);
  }

  Stage stage_ = Stage::kGoodCaptures;
};
}  // namespace SimpleChessEngine
