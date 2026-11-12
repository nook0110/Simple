#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "AspirationWindow.h"
#include "Concepts.h"
#include "DebugInfo.h"
#include "Evaluation.h"
#include "KillerTable.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "NodeCounter.h"
#include "NodeType.h"
#include "PositionFactory.h"
#include "Settings.h"
#include "StaticEvalHistory.h"
#include "SyzygyTablebase.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine {
template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
struct SearchNode;

/**
 * \brief A simple implementation of the alpha-beta search algorithm.
 *
 * \details
 * A list of features:
 *  - PVS
 *  - ZWS
 *  - Fail-soft
 *  - Transposition table
 *  - Iterative deepening
 *  - Quiescence search
 *
 *  \author nook0110
 */
class Searcher {
 public:
  template <NodeType node_type, class ExitCondition>
    requires StopSearchCondition<ExitCondition>
  friend struct SearchNode;
  friend struct SearchEvaluationTestAccess;
  constexpr static size_t kTTSizeInMb = SCE_TT_SIZE_MB;
  using SearcherTranspositionTable = TranspositionTable<kTTSizeInMb>;

  /**
   * \brief Constructor.
   *
   * \param position The initial position.
   */
  explicit Searcher(
      Position position = PositionFactory{}(),
      std::shared_ptr<SearcherTranspositionTable> transposition_table =
          std::make_shared<SearcherTranspositionTable>(),
      std::shared_ptr<const Tablebase::Syzygy> tablebase = {})
      : current_position_(std::move(position)),
        best_moves_(std::move(transposition_table)),
        tablebase_(std::move(tablebase)) {}

  /**
   * \brief Sets the current position.
   *
   * \param position New position.
   */
  void SetPosition(Position position);

  /**
   * \brief Returns the current position.
   *
   * \return The current position.
   */
  [[nodiscard]] const Position &GetPosition() const;
  void SetTablebase(std::shared_ptr<const Tablebase::Syzygy> tablebase) {
    tablebase_ = std::move(tablebase);
  }
  [[nodiscard]] const auto& GetTablebase() const { return tablebase_; }

  /**
   * \brief Returns the best move for the current position.
   *
   * \return The current best move.
   */
  [[nodiscard]] const Move &GetCurrentBestMove() const;

  /**
   * \brief Performs the alpha-beta search algorithm.
   *
   * \param max_depth max_depth for search
   * \param remaining_depth The remaining depth.
   * \param alpha The current alpha value.
   * \param beta The current beta value.
   *
   * \return Evaluation of subtree.
   */
  template <NodeType node_type, class ExitCondition>
    requires StopSearchCondition<ExitCondition>
  [[nodiscard]] SearchResult Search(const ExitCondition &stop_search_condition,
                                    Depth max_depth, Depth remaining_depth,
                                    Eval alpha, Eval beta);
  void InitStartOfSearch();
  void ClearHistories();

  struct SingularInfo {
    std::uint64_t searches{};
    std::uint64_t extensions{};
    std::uint64_t multi_cuts{};
    std::uint64_t negative_extensions{};
  };

  [[nodiscard]] const SingularInfo &GetSingularInfo() const {
    return singular_info_;
  }
  struct NullMoveInfo {
    std::uint64_t verifications{};
    std::uint64_t verification_fail_lows{};
  };

  [[nodiscard]] const NullMoveInfo &GetNullMoveInfo() const {
    return null_move_info_;
  }
  [[nodiscard]] const DebugInfo &GetInfo() const { return debug_info_; }
  [[nodiscard]] std::uint64_t GetNodes() const { return nodes_.Get(); }
  void ResetNodes() noexcept { nodes_.Reset(); }
  [[nodiscard]] const auto &GetKillers() const { return killers_; }
  [[nodiscard]] const auto &GetHistory() const { return history_; }
  [[nodiscard]] const auto &GetContinuationHistory() const {
    return *continuation_history_;
  }
  [[nodiscard]] std::int64_t GetQuietScore(const Move &move, Depth ply) const;
  [[nodiscard]] std::uint64_t GetRootEffort(const Move &move) const;
  [[nodiscard]] RootScore GetRootScore(const Move &move) const;

  using PrincipalVariation =
      InplaceVector<LegalMove, std::numeric_limits<Depth>::max()>;

  [[nodiscard]] PrincipalVariation GetPrincipalVariation(
      Depth max_depth, Position position) const;

 private:
  Age age_{};
  Move best_move_{};
  Position current_position_;     //!< Current position.
  MoveGenerator move_generator_;  //!< Move generator.
  std::shared_ptr<SearcherTranspositionTable>
      best_moves_;  //!< Shared transposition table used by Lazy SMP workers.
  std::shared_ptr<const Tablebase::Syzygy> tablebase_;
  ByPlayer<std::array<std::array<std::int64_t, kBoardArea + 1>, kBoardArea + 1>>
      history_{};
  KillerTable<2> killers_;

  struct PieceTo {
    Piece piece = Piece::kNone;
    BitIndex to = 0;
  };
  struct PlayedMove {
    PieceTo piece_to;
    Move move = Move::None();
    Player side = Player::kWhite;
    bool quiet = false;
    std::optional<Eval> parent_eval;
  };
  static constexpr std::int64_t kHistoryMax = 16384;
  static constexpr std::array<Depth, 2> kContinuationOffsets = {1, 2};

  [[nodiscard]] static std::int64_t HistoryBonus(Depth depth);
  [[nodiscard]] static constexpr std::int64_t StaticEvalHistoryBonus(Eval parent_eval,
                                                           Eval child_eval);
  [[nodiscard]] static auto &ContinuationEntry(auto &history, PieceTo previous,
                                               PieceTo current) {
    return history[previous.piece][previous.to][current.piece][current.to];
  }
  [[nodiscard]] PieceTo GetPieceTo(const Move &move) const;
  void SetPlayedMove(Depth ply, const Move &move,
                     std::optional<Eval> parent_eval);
  void SetPlayedNullMove(Depth ply);
  void UpdateStaticEvalHistory(Depth ply, Eval child_eval);
  void UpdateQuietHistory(const Move &move, Depth ply, std::int64_t bonus);
  void AddRootEffort(const Move &move, std::uint64_t nodes,
                     std::optional<Eval> score);

  template <class ExitCondition>
  void CountQuiescenceNodes(std::size_t nodes) noexcept {
    debug_info_.quiescence_nodes += nodes;
    if constexpr (ReportsNodes<ExitCondition>) nodes_.Add(nodes);
  }

  template <class Value>
  using PieceToHistory = ByPiece<std::array<Value, kBoardArea + 1>>;
  using ContinuationHistory = PieceToHistory<PieceToHistory<std::int32_t>>;
  std::unique_ptr<ContinuationHistory> continuation_history_ =
      std::make_unique<ContinuationHistory>();
  std::array<PlayedMove, kMaxSearchPly + 1> move_stack_{};
  StaticEvalHistory static_evals_;

  struct RootMoveInfo {
    Move move;
    std::uint64_t effort = 0;
    RootScore score;
  };
  std::vector<RootMoveInfo> root_effort_;

  DebugInfo debug_info_;
  NodeCounter nodes_;
  SingularInfo singular_info_;
  NullMoveInfo null_move_info_;
  Depth nmp_min_ply_ = 0;
};

}  // namespace SimpleChessEngine

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////
#include "SearchImplementation.h"

namespace SimpleChessEngine {
inline void Searcher::SetPosition(Position position) {
  current_position_ = std::move(position);
}

inline const Position &Searcher::GetPosition() const { return current_position_; }

inline const Move &Searcher::GetCurrentBestMove() const { return best_move_; }

inline Searcher::PrincipalVariation Searcher::GetPrincipalVariation(
    Depth max_depth, Position position) const {
  PrincipalVariation answer;
  for (Depth i = 0; i < max_depth; ++i) {
    const auto hashed_node = best_moves_->GetNode(position);
    if (!hashed_node.IsOccupied()) break;
    const auto legal_moves =
        MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(position);
    const auto found = std::ranges::find(legal_moves, hashed_node.move);
    if (found == legal_moves.end())
      break;
    position.DoMove(*found);
    answer.unchecked_push_back(*found);
  }
  return answer;
}

inline std::int64_t Searcher::HistoryBonus(const Depth depth) {
  return std::clamp<std::int64_t>(250 * static_cast<std::int64_t>(depth) - 200,
                                  50, 1500);
}

inline Searcher::PieceTo Searcher::GetPieceTo(const Move &move) const {
  return {current_position_.GetPieceAt(move.From()), move.To()};
}

constexpr std::int64_t Searcher::StaticEvalHistoryBonus(const Eval parent_eval,
                                                        const Eval child_eval) {
  using Policy = Settings::SearchParameters::StaticEvalPolicySettings;
  const auto gain = -(static_cast<std::int64_t>(parent_eval) + child_eval);
  const auto difference =
      std::clamp<std::int64_t>(SfMargin::RoundedFromEngine(gain),
                               Policy::kMinDifference, Policy::kMaxDifference) +
      Policy::kOffset;
  return DivideRounded(difference * Policy::kBonusMultiplier * kHistoryMax,
                       Policy::kStockfishHistoryMax);
}

inline void Searcher::SetPlayedMove(const Depth ply, const Move &move,
                                    const std::optional<Eval> parent_eval) {
  move_stack_[ply] = {GetPieceTo(move), move, current_position_.GetSideToMove(),
                      move.IsQuiet(current_position_), parent_eval};
}

inline void Searcher::SetPlayedNullMove(const Depth ply) {
  move_stack_[ply] = {};
}

inline void Searcher::UpdateStaticEvalHistory(const Depth ply,
                                              const Eval child_eval) {
  if (ply == 0) return;
  const auto &previous = move_stack_[ply - 1];
  if (!previous.quiet || !previous.parent_eval || !previous.move.IsValid())
    return;
  const auto bonus = StaticEvalHistoryBonus(*previous.parent_eval, child_eval);
  auto &entry =
      history_[previous.side][previous.move.From()][previous.move.To()];
  entry += bonus - entry * std::abs(bonus) / kHistoryMax;
}

inline std::int64_t Searcher::GetQuietScore(const Move &move,
                                            const Depth ply) const {
  const auto side = current_position_.GetSideToMove();
  std::int64_t score = history_[side][move.From()][move.To()];
  const auto current = GetPieceTo(move);
  for (const auto offset : kContinuationOffsets) {
    if (ply < offset) break;
    const auto previous = move_stack_[ply - offset].piece_to;
    if (previous.piece == Piece::kNone) continue;
    score += ContinuationEntry(*continuation_history_, previous, current);
  }
  return score;
}

inline void Searcher::UpdateQuietHistory(const Move &move, const Depth ply,
                                         const std::int64_t bonus) {
  const auto apply = [bonus](auto &entry) {
    entry += static_cast<std::remove_reference_t<decltype(entry)>>(
        bonus - entry * std::abs(bonus) / kHistoryMax);
  };
  const auto side = current_position_.GetSideToMove();
  apply(history_[side][move.From()][move.To()]);
  const auto current = GetPieceTo(move);
  for (const auto offset : kContinuationOffsets) {
    if (ply < offset) break;
    const auto previous = move_stack_[ply - offset].piece_to;
    if (previous.piece == Piece::kNone) continue;
    apply(ContinuationEntry(*continuation_history_, previous, current));
  }
}

inline std::uint64_t Searcher::GetRootEffort(const Move &move) const {
  const auto it = std::ranges::find(root_effort_, move, &RootMoveInfo::move);
  return it == root_effort_.end() ? 0 : it->effort;
}

inline RootScore Searcher::GetRootScore(const Move &move) const {
  const auto it = std::ranges::find(root_effort_, move, &RootMoveInfo::move);
  return it == root_effort_.end() ? RootScore{} : it->score;
}

inline void Searcher::AddRootEffort(const Move &move, std::uint64_t nodes,
                                    std::optional<Eval> score) {
  auto it = std::ranges::find(root_effort_, move, &RootMoveInfo::move);
  if (it == root_effort_.end()) {
    root_effort_.push_back({move, 0, {}});
    it = root_effort_.end() - 1;
  }
  it->effort += nodes;
  if (score) it->score.Update(*score);
}

inline void Searcher::InitStartOfSearch() {
  nodes_.Reset();
  nmp_min_ply_ = 0;
  killers_.Clear();
  root_effort_.clear();
  move_stack_ = {};
  static_evals_.Clear();
  for (auto &side : history_) {
    for (auto &from : side) {
      for (auto &entry : from) {
        entry = entry *
                Settings::SearchParameters::HistorySettings::kDecayNumerator /
                Settings::SearchParameters::HistorySettings::kDecayDenominator;
      }
    }
  }
}

inline void Searcher::ClearHistories() {
  history_ = {};
  *continuation_history_ = {};
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SimpleChessEngine::Searcher::Search(
    const ExitCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta) {
  debug_info_ = DebugInfo{};
  singular_info_ = {};
  null_move_info_ = {};
  ++age_;

  const auto result = SearchNode<node_type, ExitCondition>{
      *this,
      {max_depth, remaining_depth, alpha, beta},
      stop_search_condition}();
  if constexpr (!ReportsNodes<ExitCondition>) nodes_.Add(debug_info_.TotalNodes());
  return result;
}
}  // namespace SimpleChessEngine
