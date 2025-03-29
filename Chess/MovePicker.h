#pragma once
#include "MoveGenerator.h"

namespace SimpleChessEngine {
class MovePicker {
 public:
  enum class MoveType { kCapture, kQuiet, kOther };

  MovePicker() = default;
  MovePicker(MovePicker&&) = default;
  explicit MovePicker(MoveGenerator::Moves&& moves);
  ~MovePicker() = default;

  MovePicker& operator=(MovePicker&&) = default;

  MoveGenerator::Moves::iterator GetNextMove();

  void SkipMove(const Move& move);

  [[nodiscard]] bool HasMoreMoves() const;

  [[nodiscard]] MoveType GetMoveType(MoveGenerator::Moves::iterator move) const;

  [[nodiscard]] MoveGenerator::Moves::const_iterator end() const {
    return moves_.end();
  }

 private:
  MoveGenerator::Moves moves_;
  MoveGenerator::Moves::iterator current_move_;
};

MovePicker::MovePicker(MoveGenerator::Moves&& moves)
    : moves_(std::move(moves)), current_move_(moves_.begin()) {}

inline MoveGenerator::Moves::iterator MovePicker::GetNextMove() {
  /* TODO: Get best move from remaining moves */
  return current_move_++;
}
inline void MovePicker::SkipMove(const Move& move) {
  std::iter_swap(current_move_, std::find(current_move_, moves_.end(), move));
  ++current_move_;
}
inline bool MovePicker::HasMoreMoves() const {
  return current_move_ != moves_.end();
}
inline MovePicker::MoveType MovePicker::GetMoveType(
    MoveGenerator::Moves::iterator) const {
  /* TODO: implement */
  return MoveType::kOther;
}
}  // namespace SimpleChessEngine
