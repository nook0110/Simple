#include "MovePicker.h"

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <utility>

#include "Move.h"
#include "Position.h"
#include "Searcher.h"
namespace SimpleChessEngine {

namespace {
constexpr bool IsUnderpromotion(const Piece promotion) {
  return promotion != Piece::kNone && promotion != Piece::kQueen;
}

constexpr int PromotionRank(const Piece promotion) {
  switch (promotion) {
    case Piece::kQueen:
      return 4;
    case Piece::kKnight:
      return 3;
    case Piece::kRook:
      return 2;
    case Piece::kBishop:
      return 1;
    default:
      return 0;
  }
}
}  // namespace

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
  stage = MovePicker::Stage(static_cast<std::uint8_t>(stage) + 1);
  return stage;
}

MoveGenerator::Moves::const_iterator MovePicker::SelectNextMove(
    const Searcher& searcher, const Depth ply) {
  const auto& position = searcher.GetPosition();
  const auto capture_key = [this, &position](size_t idx) {
    const auto& data = data_[idx];
    return std::tuple{!IsUnderpromotion(data.promotion),
                      static_cast<int>(data.captured),
                      PromotionRank(data.promotion),
                      -static_cast<int>(position.GetPieceAt(data.from))};
  };
  const auto compare_captures = [&capture_key](size_t lhs, size_t rhs) {
    return capture_key(lhs) < capture_key(rhs);
  };
  switch (stage_) {
    case Stage::kGoodCaptures: {
      const auto is_good_capture = [this](size_t idx) {
        return data_[idx].is_good_capture;
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
      begin_quiet_ = current_move_;
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
      const auto is_quiet = [this](size_t idx) {
        return data_[idx].captured == Piece::kNone &&
               data_[idx].promotion == Piece::kNone;
      };
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
                            const Searcher& searcher, const Depth ply) {
  moves_ = std::move(moves);
  data_.resize(moves_.size());
  history_.resize(moves_.size());
  for (size_t i = 0; i < moves_.size(); ++i) {
    const auto& move = moves_[i];
    const auto from = move.From();
    const auto to = move.To();
    
    // Determine captured piece
    Piece capture = Piece::kNone;
    if (move.IsEnPassant()) {
      capture = Piece::kPawn;
    } else if (!move.IsCastling()) {
      capture = searcher.GetPosition().GetPieceAt(to);
    }
    
    const auto promotion =
        move.IsPromotion() ? move.PromotionPiece() : Piece::kNone;
    const bool tactical =
        capture != Piece::kNone || promotion == Piece::kQueen;
    data_[i] = {from, to, capture,
                tactical && !IsUnderpromotion(promotion)
                    ? searcher.GetPosition().StaticExchangeEvaluation(
                          move, kGoodCaptureThreshold)
                    : false,
                promotion};
    history_[i] = capture == Piece::kNone && promotion == Piece::kNone
                      ? searcher.GetQuietScore(move, ply)
                      : 0;
  }
  current_move_ = moves_.begin();
}

void MovePicker::SkipMove(const Move& move) {
  Swap(current_move_ - moves_.begin(),
       std::find(current_move_, moves_.end(), move) - moves_.begin());
  ++current_move_;
}

bool MovePicker::Done() const { return current_move_ == moves_.end(); }

MovePicker::Stage MovePicker::GetCurrentStage() const { return stage_; }
}  // namespace SimpleChessEngine
