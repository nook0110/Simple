#include "MovePicker.h"

#include <algorithm>
#include <cstddef>

#include "Move.h"
#include "Position.h"
#include "Searcher.h"
namespace SimpleChessEngine {

template <typename UnaryPredicate, typename Compare>
size_t FindMaxIf(size_t first, size_t last, UnaryPredicate pred, Compare comp) {
  size_t result = last;
  for (; first != last; ++first) {
    if (pred(first)) {
      if (result == last || comp(result, first)) {
        result = first;
      }
    }
  }
  return result;
}

template <typename Compare>
size_t MaxElement(size_t first, size_t last, Compare comp) {
  if (first == last) {
    return last;
  }

  size_t largest = first;
  ++first;

  for (; first != last; ++first) {
    if (comp(largest, first)) {
      largest = first;
    }
  }

  return largest;
}

MovePicker::MovePicker() = default;

MovePicker::Stage& operator++(MovePicker::Stage& stage) {
  stage = MovePicker::Stage(static_cast<uint8_t>(stage) + 1);
  return stage;
}

MoveGenerator::Moves::const_iterator MovePicker::SelectNextMove(
    const Searcher& searcher, const Depth ply) {
  const auto& position = searcher.GetPosition();
  const auto compare_captures = [this, &position](size_t lhs, size_t rhs) {
    const auto [from_lhs, to_lhs, captured_piece_lhs] = data_[lhs];
    const auto [from_rhs, to_rhs, captured_piece_rhs] = data_[rhs];
    const auto captured_idx_lhs = static_cast<int>(captured_piece_lhs);
    const auto captured_idx_rhs = static_cast<int>(captured_piece_rhs);
    const auto moving_idx_lhs = -static_cast<int>(position.GetPiece(from_lhs));
    const auto moving_idx_rhs = -static_cast<int>(position.GetPiece(from_rhs));
    return std::tie(captured_idx_lhs, moving_idx_lhs) <
           std::tie(captured_idx_rhs, moving_idx_rhs);
  };
  switch (stage_) {
    case Stage::kGoodCaptures: {
      const auto is_good_capture = [this, &position](size_t idx) {
        const auto& move = moves_[idx];
        if (std::holds_alternative<Promotion>(move) ||
            std::holds_alternative<EnCroissant>(move))
          return true;
        if (!std::holds_alternative<DefaultMove>(move)) return false;
        const auto [from, to, captured_piece] = std::get<DefaultMove>(move);
        return static_cast<size_t>(captured_piece) >=
               static_cast<size_t>(position.GetPiece(from));
      };

      auto good_capture =
          FindMaxIf(current_move_ - moves_.begin(), moves_.size(),
                    is_good_capture, compare_captures);
      if (good_capture == moves_.size()) {
        ++stage_;
      } else {
        Swap(good_capture, current_move_ - moves_.begin());
        return current_move_++;
      }
      [[fallthrough]];
    }

    case Stage::kKillers: {
      const auto& killers = searcher.GetKillers();
      for (size_t i = 0; i < killers.AvailableKillerCount(ply); ++i) {
        const auto killer = killers.Get(ply, i);
        auto killer_move = std::find(current_move_, moves_.end(), killer);
        if (killer_move != moves_.end()) {
          Swap(killer_move - moves_.begin(), current_move_ - moves_.begin());
          return current_move_++;
        }
      }
      ++stage_;
      [[fallthrough]];
    }

    case Stage::kQuiet: {
      const auto is_quiet = [this](size_t idx) { return !data_[idx].captured; };
      const auto compare_quiet = [this](size_t lhs, size_t rhs) {
        return history_[lhs] < history_[rhs];
      };

      auto quiet_move = FindMaxIf(current_move_ - moves_.begin(), moves_.size(),
                                  is_quiet, compare_quiet);
      if (quiet_move != moves_.size()) {
        Swap(quiet_move, current_move_ - moves_.begin());
        return current_move_++;
      }
      ++stage_;
      [[fallthrough]];
    }

    case Stage::kBadCaptures: {
      auto bad_capture = MaxElement(current_move_ - moves_.begin(),
                                    moves_.size(), compare_captures);
      if (bad_capture != moves_.size()) {
        Swap(bad_capture, current_move_ - moves_.begin());
        return current_move_++;
      }
      ++stage_;
      [[fallthrough]];
    }

    case Stage::kEnd: {
      return end();
    }
  }

  assert(false);
  std::unreachable();
}

void MovePicker::InitPicker(MoveGenerator::Moves&& moves,
                            const Searcher& searcher) {
  moves_ = std::move(moves);
  data_.resize(moves_.size());
  history_.resize(moves_.size());
  for (size_t i = 0; i < moves_.size(); ++i) {
    const auto [from, to, capture] = GetMoveData(moves_[i]);
    data_[i] = {from, to, capture};
    history_[i] = searcher.GetHistory()[static_cast<size_t>(
        searcher.GetPosition().GetSideToMove())][from][to];
  }
  current_move_ = moves_.begin();
}

void MovePicker::SkipMove(const Move& move) {
  Swap(current_move_ - moves_.begin(),
       std::find(current_move_, moves_.end(), move) - moves_.begin());
  ++current_move_;
}
bool MovePicker::HasMoreMoves() const { return current_move_ != moves_.end(); }
MovePicker::Stage MovePicker::GetCurrentStage() const { return stage_; }
}  // namespace SimpleChessEngine