#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

#include "Evaluation.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "Utility.h"

namespace SimpleChessEngine {
class Searcher;

class MovePicker {
 public:
  enum class Stage : std::uint8_t {
    kTT,
    kCaptureInit,
    kGoodCaptures,
    kKillers,
    kQuietInit,
    kQuiet,
    kBadCaptures,
    kEnd
  };

  void InitPicker(Position& position, const Searcher* searcher, Depth ply,
                  Move tt_move = Move::None(),
                  Move excluded_move = Move::None());
  [[nodiscard]] MaybeLegalMove SelectNextMove();
  [[nodiscard]] Stage GetCurrentStage() const { return stage_; }
  [[nodiscard]] std::ptrdiff_t MoveCount() const { return move_count_; }

 private:
  static constexpr Eval kGoodCaptureThreshold = -50;
  struct ScoredMove {
    Move move;
    std::int64_t score;
  };

  static void PartialInsertionSort(ScoredMove* begin, ScoredMove* end,
                                   std::int64_t limit);
  void Generate(bool quiet);
  [[nodiscard]] LegalMove Emit(LegalMove move);

  std::array<ScoredMove, MoveGenerator::kMaxMovesPerPosition> moves_;
  std::array<Move, 2> killers_{};
  Position* position_ = nullptr;
  const Searcher* searcher_ = nullptr;
  Move tt_move_ = Move::None();
  Move excluded_move_ = Move::None();
  Depth ply_ = 0;
  size_t current_ = 0;
  size_t end_ = 0;
  size_t bad_end_ = 0;
  size_t killer_index_ = 0;
  size_t move_count_ = 0;
  Bitboard blockers_;
  bool evasions_ = false;
  Stage stage_ = Stage::kTT;
};
}  // namespace SimpleChessEngine
