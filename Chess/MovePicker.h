#pragma once
#include "MoveGenerator.h"

namespace SimpleChessEngine {
class MovePicker {
 public:
  enum class Stage : uint8_t { kGoodCaptures, kKillers, kQuiet, kBadCaptures, kEnd };
  using History = int64_t;
  using HistoryTable = std::array<
      std::array<std::array<History, kBoardArea + 1>, kBoardArea + 1>,
      kColors>;
  struct MoveData
  {
    BitIndex from;
    BitIndex to;
    Piece moving;
    Piece captured;
    History history;
  };

  MovePicker() {
    data_.reserve(MoveGenerator::kMaxMovesPerPosition);
  }
  MovePicker(MovePicker&&) = default;
  void InitPicker(MoveGenerator::Moves&& moves, const Position& position);
  ~MovePicker() = default;

  MovePicker& operator=(MovePicker&&) = default;

  MoveGenerator::Moves::const_iterator SelectNextMove();

  void SkipMove(const Move& move);

  [[nodiscard]] bool HasMoreMoves() const;

  [[nodiscard]] MoveGenerator::Moves::const_iterator end() const {
    return moves_.end();
  }

  void Clear() {

  }

 private:
  Stage stage_;
  MoveGenerator::Moves moves_;
  std::vector<MoveData> data_;
  MoveGenerator::Moves::iterator current_move_;
  HistoryTable history_table_;
};

using Stage = MovePicker::Stage;

Stage& operator++(Stage& stage) {
  stage = Stage(static_cast<uint8_t>(stage) + 1);
  return stage;
}

void MovePicker::InitPicker(MoveGenerator::Moves&& moves,
                            const Position& position) {
  moves_ = std::move(moves);
  current_move_ = moves_.begin();
  for (const auto move : moves) {
    auto [from, to, captured_piece] = GetMoveData(move);
    data_.emplace_back(from, to, position.GetPiece(from), captured_piece,
        history_table_[static_cast<size_t>(position.GetSideToMove())][from][to]);
  }
}

inline MoveGenerator::Moves::const_iterator MovePicker::SelectNextMove() {
  switch (stage_) { 
    case Stage::kGoodCaptures: {
      for (auto it = current_move_; it != moves_.end(); ++it) {
      }
      ++stage_;
    }
    case Stage::kQuiet: {
      ++stage_;
    }
    case Stage::kBadCaptures: {
      ++stage_;
    }
    case Stage::kEnd: {
      return end();
    }
  }
  return current_move_++;
}
inline void MovePicker::SkipMove(const Move& move) {
  std::iter_swap(current_move_, std::find(current_move_, moves_.end(), move));
  ++current_move_;
}
inline bool MovePicker::HasMoreMoves() const {
  return stage_ != Stage::kEnd;
}
}  // namespace SimpleChessEngine
