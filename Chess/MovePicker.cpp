#include "MovePicker.h"

#include <algorithm>
#include <iterator>

#include "Position.h"
#include "Searcher.h"

namespace SimpleChessEngine {
namespace {
constexpr bool IsUnderpromotion(Piece promotion) {
  return promotion != Piece::kNone && promotion != Piece::kQueen;
}

constexpr int PromotionRank(Piece promotion) {
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

void MovePicker::SortByScore(ScoredRange moves) {
  if (moves.empty()) return;
  for (auto next = std::next(moves.begin()); next != moves.end(); ++next) {
    const ScoredMove inserted = *next;
    auto slot = next;
    for (; slot != moves.begin() && std::prev(slot)->score < inserted.score;
         --slot) {
      *slot = *std::prev(slot);
    }
    *slot = inserted;
  }
}

void MovePicker::InitPicker(Position& position, const Searcher* searcher,
                            Depth ply, Move tt_move, Move excluded_move) {
  position_ = &position;
  searcher_ = searcher;
  ply_ = ply;
  current_ = end_ = bad_end_ = killer_index_ = move_count_ = 0;
  killers_.fill(Move::None());
  evasions_ = position.IsUnderCheck();
  position.ComputePins(position.GetSideToMove());
  blockers_ = position.GetIrreversibleData()
                  .blockers[position.GetSideToMove()];
  excluded_move_ = excluded_move;
  tt_move_ = position.IsPseudoLegal(tt_move) ? tt_move : Move::None();
  stage_ = Stage::kTT;
}

void MovePicker::Generate(bool quiet) {
  const auto generated = [this, quiet] {
    using enum MoveGenerator::Type;
    const MoveGenerator generator;
    if (quiet) return generator.GeneratePseudoLegalMoves<kQuiets>(*position_);
    if (!searcher_ && evasions_) {
      return generator.GeneratePseudoLegalMoves<kAll>(*position_);
    }
    if (!searcher_) {
      return generator.GeneratePseudoLegalMoves<kQuiescence>(*position_);
    }
    return generator.GeneratePseudoLegalMoves<kCaptures>(*position_);
  }();
  current_ = quiet ? bad_end_ : 0;
  end_ = current_;
  for (const auto move : generated) {
    if (move == tt_move_ || move == excluded_move_ ||
        (quiet && std::ranges::find(killers_, move) != killers_.end()))
      continue;
    std::int64_t score = 0;
    if (searcher_ && quiet) {
      score = searcher_->GetQuietScore(move, ply_);
    } else if (searcher_) {
      const auto captured =
          move.IsEnPassant() ? Piece::kPawn : position_->GetPieceAt(move.To());
      const auto promotion =
          move.IsPromotion() ? move.PromotionPiece() : Piece::kNone;
      const auto victim_rank =
          !IsUnderpromotion(promotion) * 8 + static_cast<int>(captured);
      const auto promotion_rank = victim_rank * 8 + PromotionRank(promotion);
      score = promotion_rank * 8 -
              static_cast<int>(position_->GetPieceAt(move.From()));
    }
    assert(end_ < moves_.size());
    moves_[end_++] = {move, score};
  }
  if (searcher_) {
    SortByScore({moves_.begin() + current_, moves_.begin() + end_});
  }
}

LegalMove MovePicker::Emit(LegalMove move) {
  ++move_count_;
  return move;
}

MaybeLegalMove MovePicker::SelectNextMove() {
  for (;;) {
    switch (stage_) {
      case Stage::kTT:
        stage_ = Stage::kCaptureInit;
        if (tt_move_.IsValid() && position_->IsLegal(tt_move_, blockers_))
          return Emit(Position::AssumeLegal(tt_move_));
        break;
      case Stage::kCaptureInit:
        Generate(false);
        stage_ = Stage::kGoodCaptures;
        break;
      case Stage::kGoodCaptures:
        while (current_ < end_) {
          const auto candidate = moves_[current_++];
          if (!position_->IsLegal(candidate.move, blockers_)) continue;
          if (!searcher_) return Emit(Position::AssumeLegal(candidate.move));
          const auto promotion = candidate.move.IsPromotion()
                                     ? candidate.move.PromotionPiece()
                                     : Piece::kNone;
          if (!IsUnderpromotion(promotion) &&
              position_->StaticExchangeEvaluation(candidate.move,
                                                  kGoodCaptureThreshold))
            return Emit(Position::AssumeLegal(candidate.move));
          moves_[bad_end_++] = candidate;
        }
        stage_ = searcher_ ? Stage::kKillers : Stage::kEnd;
        break;
      case Stage::kKillers: {
        const auto& killers = searcher_->GetKillers();
        while (killer_index_ < killers.AvailableKillerCount(ply_)) {
          const auto index = killer_index_++;
          const auto move = killers.Get(ply_, index);
          if (move == tt_move_ || move == excluded_move_ ||
              !position_->IsPseudoLegal(move) ||
              !position_->IsLegal(move, blockers_))
            continue;
          if (!move.IsQuiet(*position_)) {
            const ScoredRange bad_captures{moves_.begin(),
                                           moves_.begin() + bad_end_};
            const auto found =
                std::ranges::find(bad_captures, move, &ScoredMove::move);
            if (found == bad_captures.end()) continue;
            found->move = Move::None();
          }
          killers_[index] = move;
          return Emit(Position::AssumeLegal(move));
        }
        stage_ = Stage::kQuietInit;
        break;
      }
      case Stage::kQuietInit:
        Generate(true);
        stage_ = Stage::kQuiet;
        break;
      case Stage::kQuiet:
        while (current_ < end_) {
          const auto move = moves_[current_++].move;
          if (position_->IsLegal(move, blockers_)) return Emit(Position::AssumeLegal(move));
        }
        current_ = 0;
        end_ = bad_end_;
        stage_ = Stage::kBadCaptures;
        break;
      case Stage::kBadCaptures:
        while (current_ < end_) {
          const auto move = moves_[current_++].move;
          if (move.IsValid()) return Emit(Position::AssumeLegal(move));
        }
        stage_ = Stage::kEnd;
        break;
      case Stage::kEnd:
        return MaybeLegalMove::None();
    }
  }
}
}  // namespace SimpleChessEngine
