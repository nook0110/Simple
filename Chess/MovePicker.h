#pragma once
#include "MoveGenerator.h"

namespace SimpleChessEngine {
class MovePicker {
 public:
  enum class MoveType { kCapture, kQuiet, kOther };

  MovePicker() = default;
  explicit MovePicker(MoveGenerator::Moves&& moves);

  MoveGenerator::Moves::iterator GetNextMove();

  void RemoveMove(const Move& move);

  [[nodiscard]] bool HasMoreMoves() const;

  [[nodiscard]] MoveType GetMoveType(MoveGenerator::Moves::iterator move) const;

  [[nodiscard]] MoveGenerator::Moves::const_iterator end() const;

  ~MovePicker() = default;

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
inline void MovePicker::RemoveMove(const Move& move) {
  moves_.erase(std::remove(current_move_, moves_.end(), move), moves_.end());
}
inline bool MovePicker::HasMoreMoves() const {
  return current_move_ != moves_.end();
}
inline MovePicker::MoveType MovePicker::GetMoveType(
    MoveGenerator::Moves::iterator) const {
  /* TODO: implement */
  assert(false);
}
inline MoveGenerator::Moves::const_iterator MovePicker::end() const {
  return moves_.end();
}
}  // namespace SimpleChessEngine
