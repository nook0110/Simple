#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "Chess/BitScan.h"
#include "Chess/MoveGenerator.h"
#include "Chess/Piece.h"
#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine {
class Searcher;

class MovePicker {
 public:
  using Moves = std::vector<Move>;
  enum class Stage : uint8_t {
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

  [[nodiscard]] bool HasMoreMoves() const;

  [[nodiscard]] Stage GetCurrentStage() const;

  [[nodiscard]] Moves::const_iterator end() const { return moves_.end(); }

 private:
  Moves moves_;
  Moves::iterator current_move_;

  struct MoveData {
    BitIndex from = {-1};
    BitIndex to = {};
    Piece captured = {};
  };

  std::vector<MoveData> data_;
  std::vector<uint64_t> history_;

  void Swap(size_t lhs, size_t rhs) {
    std::swap(moves_[lhs], moves_[rhs]);
    std::swap(data_[lhs], data_[rhs]);
    std::swap(history_[lhs], history_[rhs]);
  }

  Stage stage_ = Stage::kGoodCaptures;
};
}  // namespace SimpleChessEngine
