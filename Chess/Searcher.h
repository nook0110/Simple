#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "Concepts.h"
#include "DebugInfo.h"
#include "Evaluation.h"
#include "KillerTable.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "NodeType.h"
#include "PositionFactory.h"
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

  struct SingularInfo {
    std::uint64_t searches{};
    std::uint64_t extensions{};
    std::uint64_t multi_cuts{};
    std::uint64_t negative_extensions{};
  };

  [[nodiscard]] const SingularInfo &GetSingularInfo() const {
    return singular_info_;
  }
  [[nodiscard]] const DebugInfo &GetInfo() const { return debug_info_; }
  [[nodiscard]] const auto &GetKillers() const { return killers_; }
  [[nodiscard]] const auto &GetHistory() const { return history_; }
  [[nodiscard]] std::int64_t GetQuietScore(const Move &move, Depth ply) const;
  [[nodiscard]] std::uint64_t GetRootEffort(const Move &move) const;

  [[nodiscard]] MoveGenerator::Moves GetPrincipalVariation(
      Depth max_depth, Position position) const;

 private:
  Age age_{};
  Move best_move_{};
  Position current_position_;     //!< Current position.
  MoveGenerator move_generator_;  //!< Move generator.
  std::shared_ptr<SearcherTranspositionTable>
      best_moves_;  //!< Shared transposition table used by Lazy SMP workers.
  std::shared_ptr<const Tablebase::Syzygy> tablebase_;
  std::array<
      std::array<std::array<std::int64_t, kBoardArea + 1>, kBoardArea + 1>,
      kColors>
      history_ = {};
  KillerTable<2> killers_;

  struct PieceTo {
    Piece piece = Piece::kNone;
    BitIndex to = 0;
  };
  static constexpr std::int64_t kHistoryMax = 16384;
  static constexpr size_t kPieceToSize = kPieceTypes * (kBoardArea + 1);
  static constexpr std::array<Depth, 2> kContinuationOffsets = {1, 2};

  [[nodiscard]] static std::int64_t HistoryBonus(Depth depth);
  [[nodiscard]] static size_t ContinuationIndex(PieceTo previous,
                                                PieceTo current);
  [[nodiscard]] PieceTo GetPieceTo(const Move &move) const;
  void SetPlayedMove(Depth ply, const Move &move);
  void SetPlayedNullMove(Depth ply);
  void UpdateQuietHistory(const Move &move, Depth ply, std::int64_t bonus);
  void AddRootEffort(const Move &move, std::uint64_t nodes);

  std::vector<std::int32_t> continuation_history_ =
      std::vector<std::int32_t>(kPieceToSize * kPieceToSize);
  std::array<PieceTo, kMaxSearchPly + 1> move_stack_{};
  StaticEvalHistory static_evals_;

  std::vector<std::pair<Move, std::uint64_t>> root_effort_;

  DebugInfo debug_info_;
  SingularInfo singular_info_;
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

inline MoveGenerator::Moves Searcher::GetPrincipalVariation(Depth max_depth,
                                                     Position position) const {
  MoveGenerator::Moves answer;
  for (Depth i = 0; i < max_depth; ++i) {
    const auto hashed_node = best_moves_->GetNode(position);
    if (!hashed_node.IsOccupied() ||
        hashed_node.true_hash != position.GetHash())
      break;
    const auto legal_moves =
        MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
    if (std::ranges::find(legal_moves, hashed_node.move) == legal_moves.end())
      break;
    position.DoMove(hashed_node.move);
    answer.push_back(hashed_node.move);
  }
  return answer;
}

inline std::int64_t Searcher::HistoryBonus(const Depth depth) {
  return std::clamp<std::int64_t>(250 * static_cast<std::int64_t>(depth) - 200,
                                  50, 1500);
}

inline size_t Searcher::ContinuationIndex(const PieceTo previous,
                                          const PieceTo current) {
  const auto flatten = [](const PieceTo piece_to) {
    return static_cast<size_t>(piece_to.piece) * (kBoardArea + 1) +
           static_cast<size_t>(piece_to.to);
  };
  return flatten(previous) * kPieceToSize + flatten(current);
}

inline Searcher::PieceTo Searcher::GetPieceTo(const Move &move) const {
  return {current_position_.GetPieceAt(move.From()), move.To()};
}

inline void Searcher::SetPlayedMove(const Depth ply, const Move &move) {
  move_stack_[ply] = GetPieceTo(move);
}

inline void Searcher::SetPlayedNullMove(const Depth ply) {
  move_stack_[ply] = {};
}

inline std::int64_t Searcher::GetQuietScore(const Move &move,
                                            const Depth ply) const {
  const auto side = static_cast<size_t>(current_position_.GetSideToMove());
  std::int64_t score = history_[side][move.From()][move.To()];
  const auto current = GetPieceTo(move);
  for (const auto offset : kContinuationOffsets) {
    if (ply < offset) break;
    const auto previous = move_stack_[ply - offset];
    if (previous.piece == Piece::kNone) continue;
    score += continuation_history_[ContinuationIndex(previous, current)];
  }
  return score;
}

inline void Searcher::UpdateQuietHistory(const Move &move, const Depth ply,
                                         const std::int64_t bonus) {
  const auto apply = [bonus](auto &entry) {
    entry += static_cast<std::remove_reference_t<decltype(entry)>>(
        bonus - entry * std::abs(bonus) / kHistoryMax);
  };
  const auto side = static_cast<size_t>(current_position_.GetSideToMove());
  apply(history_[side][move.From()][move.To()]);
  const auto current = GetPieceTo(move);
  for (const auto offset : kContinuationOffsets) {
    if (ply < offset) break;
    const auto previous = move_stack_[ply - offset];
    if (previous.piece == Piece::kNone) continue;
    apply(continuation_history_[ContinuationIndex(previous, current)]);
  }
}

inline std::uint64_t Searcher::GetRootEffort(const Move &move) const {
  const auto it = std::ranges::find(root_effort_, move,
                                    &std::pair<Move, std::uint64_t>::first);
  return it == root_effort_.end() ? 0 : it->second;
}

inline void Searcher::AddRootEffort(const Move &move, std::uint64_t nodes) {
  const auto it = std::ranges::find(root_effort_, move,
                                    &std::pair<Move, std::uint64_t>::first);
  if (it == root_effort_.end()) {
    root_effort_.emplace_back(move, nodes);
    return;
  }
  it->second += nodes;
}

inline void Searcher::InitStartOfSearch() {
  killers_.Clear();
  root_effort_.clear();
  std::ranges::fill(continuation_history_, 0);
  move_stack_ = {};
  static_evals_.Clear();
  for (size_t color = 0; color < kColors; ++color) {
    for (BitIndex from = 0; from <= static_cast<BitIndex>(kBoardArea); ++from) {
      history_[color][from].fill(0LL);
    }
  }
}

template <NodeType node_type, class ExitCondition>
  requires StopSearchCondition<ExitCondition>
SearchResult SimpleChessEngine::Searcher::Search(
    const ExitCondition &stop_search_condition, Depth max_depth,
    Depth remaining_depth, Eval alpha, Eval beta) {
  debug_info_ = DebugInfo{};
  singular_info_ = {};
  ++age_;

  return SearchNode<node_type, ExitCondition>{
      *this,
      {max_depth, remaining_depth, alpha, beta},
      stop_search_condition}();
}
}  // namespace SimpleChessEngine
