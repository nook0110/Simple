#pragma once
#include <array>
#include <cassert>
#include <cstddef>
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
  void InitPicker(Moves&& moves);
  ~MovePicker() = default;

  MovePicker& operator=(MovePicker&&) = delete;
  MovePicker& operator=(const MovePicker&) = delete;

  Moves::const_iterator SelectNextMove(const Searcher& searcher,
                                       const Depth ply, const size_t color_idx);

  void SkipMove(const Move& move);

  [[nodiscard]] bool HasMoreMoves() const;

  [[nodiscard]] Stage GetCurrentStage() const;

  [[nodiscard]] Moves::const_iterator end() const { return moves_.end(); }

 private:
  Moves moves_;
  Moves::iterator current_move_;

  struct MoveData {
    BitIndex from = {};
    BitIndex to = {};
    Piece captured = {};

    bool operator==(const MoveData&) const = default;
  };

  std::array<MoveData, MoveGenerator::kMaxMovesPerPosition> data_;

  void Swap(size_t lhs, size_t rhs) {
    std::swap(moves_[lhs], moves_[rhs]);
    std::swap(data_[lhs], data_[rhs]);
  }

  const MoveData& GetData(size_t idx) {
    if (data_[idx] == MoveData{}) {
      const auto [from, to, capture] = GetMoveData(moves_[idx]);
      data_[idx] = {from, to, capture};
    }
    return data_[idx];
  }

  Stage stage_ = Stage::kGoodCaptures;
};
}  // namespace SimpleChessEngine
