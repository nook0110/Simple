#pragma once
#include <cstddef>
#include <vector>

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
  Stage stage_ = Stage::kGoodCaptures;
};
}  // namespace SimpleChessEngine
