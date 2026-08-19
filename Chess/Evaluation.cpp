#include <array>
#include <bit>
#include <cstdint>

#include "Attacks.h"
#include "Position.h"
#include "Settings.h"

namespace SimpleChessEngine {
namespace {
constexpr size_t kPawnHashSize = 4096;
static_assert(std::has_single_bit(kPawnHashSize));
constexpr size_t kKingPawnHashSize = 4096;
static_assert(std::has_single_bit(kKingPawnHashSize));

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

struct KingPawnHashEntry {
  Bitboard white_pawns{};
  Bitboard black_pawns{};
  BitIndex white_king{-1};
  BitIndex black_king{-1};
  std::array<Eval, kColors> king_score{};
};

struct KingPawnHashTable {
  size_t revision{};
  std::array<KingPawnHashEntry, kKingPawnHashSize> entries{};
};

thread_local KingPawnHashTable king_pawn_hash_table;

struct PawnEvaluation {
  TaperedEval score{};
  std::array<Eval, kColors> king_score{};
};

[[nodiscard]] const std::array<Eval, kColors>& EvaluateKingPawns(
    const Position& position, Bitboard white_pawns, Bitboard black_pawns,
    size_t pawn_hash_index);

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
    result +=
        Settings::EvaluationParameters::pawn_psqt_adjustment[relative_rank]
                                                            [file];
    if ((enemy_pawns & kPawnSpans.passed[side_index][square]).Any()) {
      continue;
    }
    result += Settings::EvaluationParameters::passed_pawn[relative_rank];
  }
  return result;
}

[[nodiscard]] PawnEvaluation EvaluatePawnStructure(const Position& position,
                                                   bool include_king_pawns) {
  const Bitboard white_pawns =
      position.GetPiecesByType<Piece::kPawn>(Player::kWhite);
  const Bitboard black_pawns =
      position.GetPiecesByType<Piece::kPawn>(Player::kBlack);

  if (pawn_hash_table.revision != Settings::EvaluationParameters::revision) {
    pawn_hash_table.entries.fill({});
    pawn_hash_table.revision = Settings::EvaluationParameters::revision;
  }

  const size_t pawn_hash_index = PawnHashIndex(white_pawns, black_pawns);
  auto& entry = pawn_hash_table.entries[pawn_hash_index];
  if (entry.white_pawns != white_pawns || entry.black_pawns != black_pawns) {
    const TaperedEval score =
        EvaluatePawns(white_pawns, black_pawns, Player::kWhite) -
        EvaluatePawns(black_pawns, white_pawns, Player::kBlack);
    entry = PawnHashEntry{white_pawns, black_pawns, score};
  }

  PawnEvaluation result{entry.score, {}};
  if (include_king_pawns) {
    result.king_score =
        EvaluateKingPawns(position, white_pawns, black_pawns, pawn_hash_index);
  }
  return result;
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
    result +=
        Settings::EvaluationParameters::psqt_adjustment[static_cast<size_t>(
            piece)][relative_rank][mirrored_file];
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

[[nodiscard]] Eval EvaluateKingPawnsUncached(const Position& position,
                                             const Player side) {
  const Player enemy = Flip(side);
  const BitIndex king_square = position.GetKingSquare(side);
  const auto [king_file, king_rank] = GetCoordinates(king_square);
  const int forward = side == Player::kWhite ? 1 : -1;
  const Bitboard own_pawns = position.GetPiecesByType<Piece::kPawn>(side);
  const Bitboard enemy_pawns = position.GetPiecesByType<Piece::kPawn>(enemy);
  Eval result = 0;

  const int first_file = std::max(0, static_cast<int>(king_file) - 1);
  const int last_file = std::min(7, static_cast<int>(king_file) + 1);
  for (int file = first_file; file <= last_file; ++file) {
    const int near_rank = static_cast<int>(king_rank) + forward;
    const int far_rank = near_rank + forward;
    if (0 <= near_rank && near_rank < kLineSize &&
        own_pawns.Test(GetSquareIndex(static_cast<File>(file),
                                      static_cast<Rank>(near_rank)))) {
      result += Settings::EvaluationParameters::king_shield_near;
    } else if (0 <= far_rank && far_rank < kLineSize &&
               own_pawns.Test(GetSquareIndex(static_cast<File>(file),
                                             static_cast<Rank>(far_rank)))) {
      result += Settings::EvaluationParameters::king_shield_far;
    }

    const Bitboard file_mask = kFileBB[static_cast<size_t>(file)];
    if ((own_pawns & file_mask).None()) {
      result += Settings::EvaluationParameters::king_semi_open_file;
      if ((enemy_pawns & file_mask).None()) {
        result += Settings::EvaluationParameters::king_open_file;
      }
    }

    int closest_storm = kLineSize;
    Bitboard storm_pawns = enemy_pawns & file_mask;
    while (storm_pawns.Any()) {
      const auto [pawn_file, pawn_rank] =
          GetCoordinates(storm_pawns.PopFirstBit());
      static_cast<void>(pawn_file);
      const int distance =
          (static_cast<int>(pawn_rank) - static_cast<int>(king_rank)) * forward;
      if (distance > 0) closest_storm = std::min(closest_storm, distance);
    }
    if (closest_storm <= 2) {
      result += Settings::EvaluationParameters::king_pawn_storm_near;
    } else if (closest_storm <= 4) {
      result += Settings::EvaluationParameters::king_pawn_storm_far;
    }
  }
  return result;
}

[[nodiscard]] const std::array<Eval, kColors>& EvaluateKingPawns(
    const Position& position, const Bitboard white_pawns,
    const Bitboard black_pawns, const size_t pawn_hash_index) {
  const BitIndex white_king = position.GetKingSquare(Player::kWhite);
  const BitIndex black_king = position.GetKingSquare(Player::kBlack);

  if (king_pawn_hash_table.revision !=
      Settings::EvaluationParameters::revision) {
    king_pawn_hash_table.entries.fill({});
    king_pawn_hash_table.revision = Settings::EvaluationParameters::revision;
  }

  const size_t king_hash =
      static_cast<size_t>(white_king) * 67 + static_cast<size_t>(black_king);
  auto& entry =
      king_pawn_hash_table
          .entries[(pawn_hash_index ^ king_hash) & (kKingPawnHashSize - 1)];
  if (entry.white_pawns != white_pawns || entry.black_pawns != black_pawns ||
      entry.white_king != white_king || entry.black_king != black_king) {
    entry = KingPawnHashEntry{
        white_pawns,
        black_pawns,
        white_king,
        black_king,
        {EvaluateKingPawnsUncached(position, Player::kWhite),
         EvaluateKingPawnsUncached(position, Player::kBlack)}};
  }
  return entry.king_score;
}

consteval std::array<std::array<Bitboard, 2>, kBoardArea>
MakeKingTropismMasks() {
  std::array<std::array<Bitboard, 2>, kBoardArea> masks{};
  for (size_t king = 0; king < kBoardArea; ++king) {
    const int king_file = static_cast<int>(king % kLineSize);
    const int king_rank = static_cast<int>(king / kLineSize);
    std::array<std::uint64_t, 2> values{};
    for (size_t target = 0; target < kBoardArea; ++target) {
      const int file = static_cast<int>(target % kLineSize);
      const int rank = static_cast<int>(target / kLineSize);
      const int file_distance =
          file > king_file ? file - king_file : king_file - file;
      const int rank_distance =
          rank > king_rank ? rank - king_rank : king_rank - rank;
      const int distance = std::max(file_distance, rank_distance);
      for (size_t ring = 0; ring < values.size(); ++ring) {
        const int radius = ring == 0 ? 2 : 4;
        if (distance <= radius) {
          values[ring] |= std::uint64_t{1} << target;
        }
      }
    }
    for (size_t ring = 0; ring < values.size(); ++ring) {
      masks[king][ring] = Bitboard{values[ring]};
    }
  }
  return masks;
}

inline constexpr auto kKingTropismMasks = MakeKingTropismMasks();

template <Piece piece>
void AddKingTropism(const Position& position, const Player attacker,
                    const BitIndex king_square, size_t& attacker_count,
                    Eval& attack_units) {
  const Bitboard pieces = position.GetPiecesByType<piece>(attacker);
  const auto& masks = kKingTropismMasks[king_square];
  const size_t outer = (pieces & masks[1]).Count();
  attacker_count += outer;
  const size_t tropism = outer + (pieces & masks[0]).Count();
  attack_units +=
      static_cast<Eval>(tropism) *
      Settings::EvaluationParameters::king_attack[static_cast<size_t>(piece)];
}

[[nodiscard]] Eval EvaluateKingAttack(const Position& position,
                                      const Player defender) {
  static constexpr std::array<int, 8> kAttackerWeight = {0,  0,  50, 75,
                                                         88, 94, 97, 99};
  const Player attacker = Flip(defender);
  const BitIndex king_square = position.GetKingSquare(defender);
  size_t attacker_count = 0;
  Eval attack_units = 0;
  AddKingTropism<Piece::kKnight>(position, attacker, king_square,
                                 attacker_count, attack_units);
  AddKingTropism<Piece::kBishop>(position, attacker, king_square,
                                 attacker_count, attack_units);
  AddKingTropism<Piece::kRook>(position, attacker, king_square, attacker_count,
                               attack_units);
  AddKingTropism<Piece::kQueen>(position, attacker, king_square, attacker_count,
                                attack_units);
  const size_t weight_index =
      std::min(attacker_count, kAttackerWeight.size() - 1);
  return attack_units * kAttackerWeight[weight_index] / 100;
}

[[nodiscard]] TaperedEval EvaluateKingSafety(const Position& position,
                                             const Player side,
                                             const Eval king_pawn_score) {
  if (position.GetPiecesByType<Piece::kQueen>(Flip(side)).None()) return {};
  Eval middlegame = 0;
  if (Settings::EvaluationParameters::king_pawns_enabled) {
    middlegame += king_pawn_score;
  }
  if (Settings::EvaluationParameters::king_attacks_enabled) {
    middlegame -= EvaluateKingAttack(position, side);
  }
  return TaperedEval{{middlegame, 0}};
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
  const bool king_safety_active =
      Settings::EvaluationParameters::king_safety_enabled &&
      (GetPiecesByType<Piece::kQueen>(Player::kWhite) |
       GetPiecesByType<Piece::kQueen>(Player::kBlack))
          .Any();
  const bool include_king_pawns =
      king_safety_active && Settings::EvaluationParameters::king_pawns_enabled;
  PawnEvaluation pawn_evaluation{};
  if (Settings::EvaluationParameters::pawns_enabled || include_king_pawns) {
    pawn_evaluation = EvaluatePawnStructure(*this, include_king_pawns);
  }
  if (Settings::EvaluationParameters::pawns_enabled) {
    result += us == Player::kWhite ? pawn_evaluation.score
                                   : TaperedEval{} - pawn_evaluation.score;
  }
  if (Settings::EvaluationParameters::mobility_enabled) {
    const TaperedEval mobility = EvaluateMobility(*this, Player::kWhite) -
                                 EvaluateMobility(*this, Player::kBlack);
    result += us == Player::kWhite ? mobility : TaperedEval{} - mobility;
  }
  if (Settings::EvaluationParameters::psqt_adjustment_enabled) {
    const TaperedEval adjustment =
        EvaluatePsqtAdjustments(*this, Player::kWhite) -
        EvaluatePsqtAdjustments(*this, Player::kBlack);
    result += us == Player::kWhite ? adjustment : TaperedEval{} - adjustment;
  }
  if (king_safety_active) {
    if (Settings::EvaluationParameters::king_pawns_enabled) {
      result +=
          EvaluateKingSafety(*this, us, pawn_evaluation.king_score[us_idx]) -
          EvaluateKingSafety(*this, them, pawn_evaluation.king_score[them_idx]);
    } else {
      result +=
          EvaluateKingSafety(*this, us, 0) - EvaluateKingSafety(*this, them, 0);
    }
  }

  const Eval tapered = result(evaluation_data_.non_pawn_material);

  return tapered + kTempoBonus;
}
}  // namespace SimpleChessEngine
