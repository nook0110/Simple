#include "Position.h"

#include <array>
#include <bit>
#include <cstdint>

#include "Attacks.h"
#include "Settings.h"

namespace SimpleChessEngine {
namespace {
constexpr size_t kPawnHashSize = 4096;
static_assert(std::has_single_bit(kPawnHashSize));

struct PawnHashEntry {
  Bitboard white_pawns{};
  Bitboard black_pawns{};
  TaperedEval score{};
};

struct PawnHashTable {
  size_t revision{};
  std::array<PawnHashEntry, kPawnHashSize> entries{};
};

thread_local PawnHashTable pawn_hash_table;

[[nodiscard]] size_t PawnHashIndex(const Bitboard white_pawns,
                                   const Bitboard black_pawns) {
  std::uint64_t hash = static_cast<std::uint64_t>(white_pawns) ^
                       std::rotl(static_cast<std::uint64_t>(black_pawns), 32);
  hash ^= hash >> 30;
  hash *= 0xbf58476d1ce4e5b9ULL;
  hash ^= hash >> 27;
  hash *= 0x94d049bb133111ebULL;
  hash ^= hash >> 31;
  return static_cast<size_t>(hash) & (kPawnHashSize - 1);
}

[[nodiscard]] TaperedEval EvaluatePawns(const Bitboard pawns,
                                        const Bitboard enemy_pawns,
                                        const Player side) {
  const auto side_index = static_cast<size_t>(side);
  TaperedEval result{};

  Bitboard remaining = pawns;
  while (remaining.Any()) {
    const BitIndex square = remaining.PopFirstBit();
    if ((pawns & kPawnSpans.rear[side_index][square]).Any()) {
      result += Settings::EvaluationParameters::doubled_pawn;
    }
    if ((pawns & kPawnSpans.adjacent_files[square]).None()) {
      result += Settings::EvaluationParameters::isolated_pawn;
    }
    const auto [file_coordinate, rank_coordinate] = GetCoordinates(square);
    const auto file = static_cast<size_t>(file_coordinate);
    const auto rank = static_cast<size_t>(rank_coordinate);
    const auto relative_rank =
        side == Player::kWhite ? rank : static_cast<size_t>(7 - rank);
    result += Settings::EvaluationParameters::
        pawn_psqt_adjustment[relative_rank][file];
    if ((enemy_pawns & kPawnSpans.passed[side_index][square]).Any()) {
      continue;
    }
    result += Settings::EvaluationParameters::passed_pawn[relative_rank];
  }
  return result;
}

[[nodiscard]] TaperedEval EvaluatePawnStructure(const Position& position) {
  const Bitboard white_pawns =
      position.GetPiecesByType<Piece::kPawn>(Player::kWhite);
  const Bitboard black_pawns =
      position.GetPiecesByType<Piece::kPawn>(Player::kBlack);

  if (pawn_hash_table.revision !=
      Settings::EvaluationParameters::revision) {
    pawn_hash_table.entries.fill({});
    pawn_hash_table.revision = Settings::EvaluationParameters::revision;
  }

  auto& entry = pawn_hash_table.entries[PawnHashIndex(white_pawns, black_pawns)];
  if (entry.white_pawns == white_pawns && entry.black_pawns == black_pawns) {
    return entry.score;
  }

  const TaperedEval score =
      EvaluatePawns(white_pawns, black_pawns, Player::kWhite) -
      EvaluatePawns(black_pawns, white_pawns, Player::kBlack);
  entry = PawnHashEntry{white_pawns, black_pawns, score};
  return score;
}

template <Piece piece>
void AddMobility(const Position& position, const Player side,
                 TaperedEval& result) {
  const Bitboard occupied = position.GetAllPieces();
  const Bitboard own_pieces = position.GetPieces(side);
  Bitboard pieces = position.GetPiecesByType<piece>(side);
  size_t mobility = 0;
  while (pieces.Any()) {
    const BitIndex square = pieces.PopFirstBit();
    mobility +=
        (AttackTable<piece>::GetAttackMap(square, occupied) & ~own_pieces)
            .Count();
  }
  const auto& weight =
      Settings::EvaluationParameters::mobility[static_cast<size_t>(piece)];
  result.eval[0] += static_cast<Eval>(mobility) * weight.eval[0];
  result.eval[1] += static_cast<Eval>(mobility) * weight.eval[1];
}

[[nodiscard]] TaperedEval EvaluateMobility(const Position& position,
                                           const Player side) {
  TaperedEval result{};
  AddMobility<Piece::kKnight>(position, side, result);
  AddMobility<Piece::kBishop>(position, side, result);
  AddMobility<Piece::kRook>(position, side, result);
  AddMobility<Piece::kQueen>(position, side, result);
  return result;
}

template <Piece piece>
void AddPsqtAdjustment(const Position& position, const Player side,
                       TaperedEval& result) {
  Bitboard pieces = position.GetPiecesByType<piece>(side);
  while (pieces.Any()) {
    const BitIndex square = pieces.PopFirstBit();
    const auto [file_coordinate, rank_coordinate] = GetCoordinates(square);
    const size_t file = static_cast<size_t>(file_coordinate);
    const size_t rank = static_cast<size_t>(rank_coordinate);
    const size_t relative_rank =
        side == Player::kWhite ? rank : static_cast<size_t>(7 - rank);
    const size_t mirrored_file = std::min(file, static_cast<size_t>(7 - file));
    result += Settings::EvaluationParameters::psqt_adjustment
        [static_cast<size_t>(piece)][relative_rank][mirrored_file];
  }
}

[[nodiscard]] TaperedEval EvaluatePsqtAdjustments(const Position& position,
                                                  const Player side) {
  TaperedEval result{};
  AddPsqtAdjustment<Piece::kKnight>(position, side, result);
  AddPsqtAdjustment<Piece::kBishop>(position, side, result);
  AddPsqtAdjustment<Piece::kRook>(position, side, result);
  AddPsqtAdjustment<Piece::kQueen>(position, side, result);
  AddPsqtAdjustment<Piece::kKing>(position, side, result);
  return result;
}
}  // namespace

[[nodiscard]] Eval TaperedEval::operator()(PhaseValue pv) const {
  const auto mg_limit =
      kPhaseValueLimits[static_cast<size_t>(GamePhase::kMiddleGame)];
  const auto eg_limit =
      kPhaseValueLimits[static_cast<size_t>(GamePhase::kEndGame)];
  pv = std::clamp(pv, eg_limit, mg_limit);
  return (eval[static_cast<size_t>(GamePhase::kMiddleGame)] * (pv - eg_limit) +
          eval[static_cast<size_t>(GamePhase::kEndGame)] * (mg_limit - pv)) /
         kLimitsDifference;
}

[[nodiscard]] Eval Position::Evaluate() const {
  const auto us = side_to_move_;
  const auto them = Flip(us);
  const auto us_idx = static_cast<size_t>(us);
  const auto them_idx = static_cast<size_t>(them);

  TaperedEval result{};
  result +=
      evaluation_data_.material[us_idx] - evaluation_data_.material[them_idx];
  if (Settings::EvaluationParameters::material_enabled) {
    for (size_t piece = static_cast<size_t>(Piece::kPawn);
         piece <= static_cast<size_t>(Piece::kQueen); ++piece) {
      const int count_difference =
          static_cast<int>(evaluation_data_.piece_counts[us_idx][piece]) -
          static_cast<int>(evaluation_data_.piece_counts[them_idx][piece]);
      const TaperedEval correction =
          Settings::EvaluationParameters::material_value[piece] -
          kPieceValues[piece];
      result.eval[0] += count_difference * correction.eval[0];
      result.eval[1] += count_difference * correction.eval[1];
    }
  }
  result += evaluation_data_.psqt[us_idx] - evaluation_data_.psqt[them_idx];
  if (Settings::EvaluationParameters::pawns_enabled) {
    const TaperedEval pawn_score = EvaluatePawnStructure(*this);
    result += us == Player::kWhite ? pawn_score : TaperedEval{} - pawn_score;
  }
  if (Settings::EvaluationParameters::mobility_enabled) {
    const TaperedEval mobility =
        EvaluateMobility(*this, Player::kWhite) -
        EvaluateMobility(*this, Player::kBlack);
    result += us == Player::kWhite ? mobility : TaperedEval{} - mobility;
  }
  if (Settings::EvaluationParameters::psqt_adjustment_enabled) {
    const TaperedEval adjustment =
        EvaluatePsqtAdjustments(*this, Player::kWhite) -
        EvaluatePsqtAdjustments(*this, Player::kBlack);
    result += us == Player::kWhite ? adjustment
                                   : TaperedEval{} - adjustment;
  }

  const Eval tapered = result(evaluation_data_.non_pawn_material);

  return tapered + kTempoBonus;
}
}  // namespace SimpleChessEngine
