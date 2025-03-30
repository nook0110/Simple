#include "MovePicker.h"

#include <algorithm>

#include "Position.h"
#include "Searcher.h"
namespace SimpleChessEngine {

template <typename InputIt, typename UnaryPredicate, typename Compare>
InputIt FindMaxIf(InputIt first, InputIt last, UnaryPredicate pred,
                  Compare comp) {
  InputIt result = last;
  for (; first != last; ++first) {
    if (pred(*first)) {
      if (result == last || comp(*result, *first)) {
        result = first;
      }
    }
  }
  return result;
}

MovePicker::MovePicker() = default;

MovePicker::Stage& operator++(MovePicker::Stage& stage) {
  stage = MovePicker::Stage(static_cast<uint8_t>(stage) + 1);
  return stage;
}

MoveGenerator::Moves::const_iterator MovePicker::SelectNextMove(
    const Searcher& searcher, const Depth ply, const size_t color_idx) {
  const auto& position = searcher.GetPosition();
  const static auto compare_captures = [&position](const Move& lhs,
                                                   const Move& rhs) {
    const auto [from_lhs, to_lhs, captured_piece_lhs] = GetMoveData(lhs);
    const auto [from_rhs, to_rhs, captured_piece_rhs] = GetMoveData(rhs);
    const auto captured_idx_lhs = static_cast<int>(captured_piece_lhs);
    const auto captured_idx_rhs = static_cast<int>(captured_piece_rhs);
    const auto moving_idx_lhs = -static_cast<int>(position.GetPiece(from_lhs));
    const auto moving_idx_rhs = -static_cast<int>(position.GetPiece(from_rhs));
    return std::tie(captured_idx_lhs, moving_idx_lhs) >
           std::tie(captured_idx_rhs, moving_idx_rhs);
  };
  switch (stage_) {
    case Stage::kGoodCaptures: {
      const static auto is_good_capture = [this, &position](const Move& move) {
        if (std::holds_alternative<Promotion>(move) ||
            std::holds_alternative<EnCroissant>(move))
          return true;
        if (!std::holds_alternative<DefaultMove>(move)) return false;
        const auto [from, to, captured_piece] = std::get<DefaultMove>(move);
        return static_cast<size_t>(captured_piece) >=
               static_cast<size_t>(position.GetPiece(from));
      };

      auto good_capture = FindMaxIf(current_move_, moves_.end(),
                                    is_good_capture, compare_captures);
      if (good_capture == moves_.end()) {
        ++stage_;
        return SelectNextMove(searcher, ply, color_idx);
      }
      std::iter_swap(good_capture, current_move_);
      return current_move_++;
    }

    case Stage::kKillers: {
      const auto& killers = searcher.GetKillers();
      for (size_t i = 0; i < killers.AvailableKillerCount(ply); ++i) {
        const auto killer = killers.Get(ply, i);
        auto killer_move = std::find(current_move_, moves_.end(), killer);
        if (killer_move != moves_.end()) {
          std::iter_swap(killer_move, current_move_);
          return current_move_++;
        }
      }
      ++stage_;
      return SelectNextMove(searcher, ply, color_idx);
    }

    case Stage::kQuiet: {
      const auto& history = searcher.GetHistory();
      const static auto is_quiet = [](const Move& move) {
        return IsQuiet(move);
      };
      const auto compare_quite = [&history, color_idx](const Move& lhs,
                                                       const Move& rhs) {
        const auto [from_lhs, to_lhs, captured_piece_lhs] = GetMoveData(lhs);
        const auto [from_rhs, to_rhs, captured_piece_rhs] = GetMoveData(rhs);
        return history[color_idx][from_lhs][to_lhs] >
               history[color_idx][from_rhs][to_rhs];
      };

      auto quiet_move =
          FindMaxIf(current_move_, moves_.end(), is_quiet, compare_quite);
      if (quiet_move != moves_.end()) {
        std::iter_swap(quiet_move, current_move_);
        return current_move_++;
      }
      ++stage_;
      return SelectNextMove(searcher, ply, color_idx);
    }

    case Stage::kBadCaptures: {
      auto bad_capture =
          std::max_element(current_move_, moves_.end(), compare_captures);
      if (bad_capture != moves_.end()) {
        std::iter_swap(bad_capture, current_move_);
        return current_move_++;
      }
      ++stage_;
      return SelectNextMove(searcher, ply, color_idx);
    }

    case Stage::kEnd: {
      return end();
    }
  }

  assert(false);
}

void MovePicker::InitPicker(MoveGenerator::Moves&& moves) {
  moves_ = std::move(moves);
  current_move_ = moves_.begin();
}

void MovePicker::SkipMove(const Move& move) {
  std::iter_swap(current_move_, std::find(current_move_, moves_.end(), move));
  ++current_move_;
}
bool MovePicker::HasMoreMoves() const { return current_move_ != moves_.end(); }
MovePicker::Stage MovePicker::GetCurrentStage() const { return stage_; }
}  // namespace SimpleChessEngine