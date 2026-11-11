#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "ExitCondition.h"
#include "MoveFactory.h"
#include "Nnue/Nnue.h"
#include "PositionFactory.h"
#include "Quiescence.h"
#include "Searcher.h"

using namespace SimpleChessEngine;

TEST(QuiescenceDraws, ExpiredFiftyMoveClock) {
  auto position = PositionFactory{}("7k/8/6K1/8/8/8/8/1Q6 w - - 100 51");
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kDrawValue);
}

TEST(QuiescenceDraws, RepetitionInGameHistory) {
  auto position = PositionFactory{}("7k/8/6K1/8/8/8/8/1Q6 w - - 0 1");
  for (const auto* move : {"b1b2", "h8g8", "b2b1", "g8h8", "b1b2",
                           "h8g8", "b2b1", "g8h8"}) {
    position.DoMove(MoveFactory{}(position, move));
  }
  ASSERT_TRUE(position.DetectRepetition(0));
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kDrawValue);
}

TEST(QuiescenceDraws, StalemateWithPinnedBishop) {
  auto position = PositionFactory{}("7k/5K1b/7Q/8/8/8/8/8 b - - 0 1");
  ASSERT_FALSE(position.IsUnderCheck());
  ASSERT_TRUE(MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position)
                  .empty());
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kDrawValue);
}

TEST(QuiescenceDraws, MateOverridesExpiredFiftyMoveClock) {
  auto position = PositionFactory{}("7k/6Q1/6K1/8/8/8/8/8 b - - 100 51");
  DepthCondition condition{4};
  Quiescence search{condition};
  EXPECT_EQ(search.Search<true>(position, kMateValue, -kMateValue, 1),
            kMateValue + 1);
}

TEST(TranspositionTable, LaterMoveCutoffStoresCurrentScore) {
  const auto position = PositionFactory{}();
  DepthCondition condition{2};
  size_t cutoffs = 0;
  for (Eval beta = -100; beta <= 100; beta += 2) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    Searcher searcher(position, table);
    searcher.InitStartOfSearch();
    const auto result =
        searcher.Search<NodeType::kPV>(condition, 2, 2, beta - 1, beta);
    ASSERT_TRUE(result.has_value());
    const auto node = table->GetNode(position);
    if (node.IsOccupied() && node.bound == Bound::kLower) {
      ++cutoffs;
      EXPECT_GE(node.score, beta) << "beta=" << beta;
    }
  }
  EXPECT_GT(cutoffs, 0u);
}

TEST(SingularExtensions, ExclusionIgnoresBoundsAndPreservesFullNodeEntry) {
  const auto position = PositionFactory{}();
  const auto excluded = MoveFactory{}(position, "e2e4");
  DepthCondition condition{8};
  for (const auto bound : {Bound::kLower, Bound::kExact, Bound::kUpper}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    table->SetEntry(position, excluded, 10000, 3, bound, 0);
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kAll, DepthCondition>{
        searcher, {8, 3, 499, 500, 1}, condition, excluded}();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 499);
    EXPECT_GT(searcher.GetInfo().searched_nodes, 1u);
    const auto node = table->GetNode(position);
    EXPECT_EQ(node.move, excluded);
    EXPECT_EQ(node.score, 10000);
    EXPECT_EQ(node.depth, 3);
    EXPECT_EQ(node.bound, bound);
    EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
    EXPECT_EQ(searcher.GetSingularInfo().searches, 0u);
  }
}

TEST(SingularExtensions, ExcludingOnlyEvasionReturnsFailLow) {
  auto position = PositionFactory{}("r7/8/8/8/8/2k5/8/K7 w - - 0 1");
  const auto moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kAll>(position);
  ASSERT_EQ(moves.size(), 1u);
  ASSERT_TRUE(position.IsUnderCheck());
  Searcher searcher(position);
  DepthCondition condition{8};
  const auto result = SearchNode<NodeType::kAll, DepthCondition>{
      searcher, {8, 3, 499, 500, 1}, condition, moves.front()}();
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, 499);
}

TEST(SingularExtensions, ExtendsOnlyEligibleLowerBounds) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  DepthCondition condition{8};
  struct Case {
    Bound bound;
    Depth depth;
    Eval score;
    bool eligible;
  };
  for (const auto test : {
           Case{Bound::kLower, 4, 10000, true},
           Case{Bound::kExact, 4, 10000, true},
           Case{Bound::kUpper, 4, 10000, false},
           Case{Bound::kLower, 3, 10000, false},
           Case{Bound::kLower, 4, -kMateValue - 10, false},
           Case{Bound::kLower, 4, kTablebaseWinValue - 10, false}}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    table->SetEntry(position, move, test.score, test.depth, test.bound, 0);
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kPV, DepthCondition>{
        searcher, {8, 7, -20000, 20000, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(searcher.GetSingularInfo().extensions > 0, test.eligible);
  }
}

TEST(SingularExtensions, MultiCutAndNegativeExtension) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  DepthCondition condition{8};
  for (const Eval beta : {-11000, -10001}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    table->SetEntry(position, move, -10000, 4, Bound::kLower, 0);
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kPV, DepthCondition>{
        searcher, {8, 7, beta - 1, beta, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, beta);
    EXPECT_EQ(searcher.GetSingularInfo().multi_cuts, beta == -11000 ? 1u : 0u);
    EXPECT_EQ(searcher.GetSingularInfo().negative_extensions,
              beta == -10001 ? 1u : 0u);
    EXPECT_EQ(searcher.GetSingularInfo().extensions, 0u);
  }
}

TEST(SingularExtensions, CancelledSearchPreservesEntryAndPosition) {
  auto stop = std::make_shared<std::atomic_bool>(true);
  DepthCondition condition{8, stop};
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  table->SetEntry(position, move, 10000, 4, Bound::kLower, 0);
  Searcher searcher(position, table);
  const auto result = SearchNode<NodeType::kPV, DepthCondition>{
      searcher, {8, 7, -20000, 20000, 1}, condition}();
  EXPECT_FALSE(result);
  EXPECT_EQ(searcher.GetSingularInfo().searches, 0u);
  EXPECT_EQ(table->GetNode(position).score, 10000);
  EXPECT_EQ(table->GetNode(position).depth, 4);
  EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
}

TEST(SingularExtensions, FindsMateInTwoAndThree) {
  for (const auto& [fen, distance] : {
           std::pair{"7k/8/5K2/8/8/8/8/1Q6 w - - 0 1", 3},
           std::pair{"7k/8/8/4K3/8/8/8/1Q6 w - - 0 1", 5}}) {
    Searcher searcher(PositionFactory{}(fen));
    searcher.InitStartOfSearch();
    DepthCondition condition{8};
    SearchResult score;
    for (Depth depth = 1; depth <= 8; ++depth) {
      score = searcher.Search<NodeType::kPV>(condition, depth, depth,
                                           kMateValue, -kMateValue);
      ASSERT_TRUE(score);
    }
    EXPECT_EQ(*score, -kMateValue - distance) << fen;
  }
}

TEST(SingularExtensions, ReportsTriggerCounts) {
  const auto* net = std::getenv("SCE_SINGULAR_STATS_NET");
  if (!net) {
    GTEST_SKIP() << "Set SCE_SINGULAR_STATS_NET for NNUE trigger measurements";
  }
  ASSERT_TRUE(Nnue::Load(net));
  struct NetworkGuard {
    ~NetworkGuard() { Nnue::Unload(); }
  } guard;
  for (const auto* fen : {
           "r1bqk2r/pp2np1p/2n1p1pb/3pP3/3P1P2/5N2/PP2B1PP/RNBQK2R w KQkq - 0 1",
           "r1bqk2r/pp1pppbp/2n2np1/2p5/2P5/3PPN2/PP3PPP/RNBQKB1R b KQkq - 0 1",
           "r2qkbnr/ppp3p1/2np3p/4p2b/2B1P3/1P3N1P/PBP2PP1/RN1QK2R w KQkq - 0 1"}) {
    Searcher searcher(PositionFactory{}(fen));
    searcher.InitStartOfSearch();
    DepthCondition condition{11};
    std::uint64_t nodes = 0, extensions = 0, multi_cuts = 0;
    for (Depth depth = 1; depth <= 11; ++depth) {
      const auto score = searcher.Search<NodeType::kPV>(
          condition, depth, depth, kMateValue, -kMateValue);
      ASSERT_TRUE(score);
      nodes += searcher.GetInfo().searched_nodes +
               searcher.GetInfo().quiescence_nodes;
      extensions += searcher.GetSingularInfo().extensions;
      multi_cuts += searcher.GetSingularInfo().multi_cuts;
    }
    EXPECT_GT(extensions, 0u);
    std::cout << "singular stats: " << fen << " nodes=" << nodes
              << " extensions=" << extensions << " multi_cuts=" << multi_cuts
              << " extensions_per_1M=" << 1000000.0 * extensions / nodes
              << " multi_cuts_per_1M=" << 1000000.0 * multi_cuts / nodes << '\n';
  }
}

namespace SimpleChessEngine {
struct ProbCutTestAccess {
  template <NodeType node_type = NodeType::kAll, class ExitCondition>
  static std::optional<SearchResult> Run(
      Searcher& searcher, const ExitCondition& condition,
      SearchState state = {8, 5, -96, -95, 1}, bool improving = false,
      std::optional<Move> excluded = std::nullopt,
      std::optional<Eval> static_eval = std::nullopt) {
    SearchNode<node_type, ExitCondition> node{searcher, state, condition,
                                              excluded};
    node.ProbeTranspositionTable();
    node.evaluation_trend_.improving = improving;
    node.position_info_.static_eval = static_eval;
    return node.CheckProbCut();
  }
};
}

namespace {
constexpr auto kProbCutFen = "k7/8/8/3p4/3R4/8/8/K7 w - - 0 1";

void ExpectProbCutPositionRestored(const Position& actual,
                                   const Position& expected) {
  EXPECT_EQ(actual, expected);
  EXPECT_EQ(actual.Evaluate(), expected.Evaluate());
  EXPECT_EQ(actual.GetHalfMoveClock(), expected.GetHalfMoveClock());
  EXPECT_EQ(actual.DetectRepetition(1), expected.DetectRepetition(1));
}
}

TEST(ProbCut, BothSearchesPassAndReturnOriginalBetaWithoutParentEntry) {
  const auto position = PositionFactory{}(kProbCutFen);
  DepthCondition condition{8};
  for (const auto node_type : {NodeType::kAll, NodeType::kCut}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    Searcher searcher(position, table);
    const auto result =
        node_type == NodeType::kAll
            ? ProbCutTestAccess::Run<NodeType::kAll>(searcher, condition)
            : ProbCutTestAccess::Run<NodeType::kCut>(searcher, condition);
    ASSERT_TRUE(result);
    ASSERT_TRUE(*result);
    EXPECT_EQ(**result, -95);
    const auto& info = searcher.GetInfo();
    EXPECT_EQ(info.probcut_tries, 1u);
    EXPECT_EQ(info.probcut_candidates, 1u);
    EXPECT_EQ(info.probcut_qsearch_passes, 1u);
    EXPECT_EQ(info.probcut_cuts, 1u);
    EXPECT_GT(info.probcut_verification_nodes, 0u);
    EXPECT_LE(info.probcut_qsearch_nodes, info.quiescence_nodes);
    EXPECT_FALSE(table->GetNode(position).IsOccupied());
    ExpectProbCutPositionRestored(searcher.GetPosition(), position);
  }
}

TEST(ProbCut, QuiescencePassesButPositiveDepthVerificationRejects) {
  const auto position =
      PositionFactory{}("7r/6k1/8/8/4p3/2N5/PP6/K7 w - - 0 1");
  Searcher searcher(position);
  DepthCondition condition{8};
  EXPECT_FALSE(
      ProbCutTestAccess::Run(searcher, condition, {8, 5, -1096, -1095, 1}));
  const auto& info = searcher.GetInfo();
  EXPECT_EQ(info.probcut_candidates, 1u);
  EXPECT_EQ(info.probcut_qsearch_passes, 1u);
  EXPECT_GT(info.probcut_verification_nodes, 0u);
  EXPECT_EQ(info.probcut_cuts, 0u);
  ExpectProbCutPositionRestored(searcher.GetPosition(), position);
}

TEST(ProbCut, QuiescenceRejectionDoesNotVerify) {
  const auto position = PositionFactory{}(kProbCutFen);
  Searcher searcher(position);
  DepthCondition condition{8};
  EXPECT_FALSE(ProbCutTestAccess::Run(searcher, condition, {8, 5, 904, 905, 1},
                                      false, std::nullopt, 1000));
  const auto& info = searcher.GetInfo();
  EXPECT_EQ(info.probcut_candidates, 1u);
  EXPECT_EQ(info.probcut_qsearch_passes, 0u);
  EXPECT_EQ(info.probcut_verification_nodes, 0u);
  EXPECT_EQ(info.probcut_cuts, 0u);
  EXPECT_GT(info.probcut_qsearch_nodes, 0u);
  ExpectProbCutPositionRestored(searcher.GetPosition(), position);
}

TEST(ProbCut, NoTacticalCandidate) {
  const auto position = PositionFactory{}();
  Searcher searcher(position);
  DepthCondition condition{8};
  EXPECT_FALSE(ProbCutTestAccess::Run(searcher, condition));
  EXPECT_EQ(searcher.GetInfo().probcut_tries, 1u);
  EXPECT_EQ(searcher.GetInfo().probcut_candidates, 0u);
  EXPECT_EQ(searcher.GetInfo().quiescence_nodes, 0u);
  ExpectProbCutPositionRestored(searcher.GetPosition(), position);
}

TEST(ProbCut, SkipsRootPvCheckExclusionShallowAndDecisiveWindows) {
  const auto position = PositionFactory{}(kProbCutFen);
  DepthCondition condition{8};
  const auto excluded = MoveFactory{}(position, "d4d5");
  constexpr Eval decisive = kTablebaseWinValue - kMaxSearchPly;
  for (const auto state :
       {SearchState{8, 5, -96, -95, 0}, SearchState{8, 4, -96, -95, 1},
        SearchState{8, 5, kMateValue, kMateValue + 1, 1},
        SearchState{8, 5, -kMateValue - 2, -kMateValue - 1, 1},
        SearchState{8, 5, decisive - 1, decisive, 1},
        SearchState{8, 5, -decisive - 1, -decisive, 1},
        SearchState{8, 5, decisive - 96, decisive - 95, 1},
        SearchState{8, 5, decisive - 50, decisive - 49, 1}}) {
    Searcher searcher(position);
    EXPECT_FALSE(ProbCutTestAccess::Run(searcher, condition, state));
    EXPECT_EQ(searcher.GetInfo().probcut_tries, 0u);
  }
  for (const auto pv : {false, true}) {
    Searcher searcher(position);
    if (pv) {
      EXPECT_FALSE(ProbCutTestAccess::Run<NodeType::kPV>(searcher, condition));
    } else {
      EXPECT_FALSE(ProbCutTestAccess::Run(
          searcher, condition, {8, 5, -96, -95, 1}, false, excluded));
    }
    EXPECT_EQ(searcher.GetInfo().probcut_tries, 0u);
  }
  Searcher checked(PositionFactory{}("k3r3/8/8/8/8/8/P7/4K3 w - - 0 1"));
  ASSERT_TRUE(checked.GetPosition().IsUnderCheck());
  EXPECT_FALSE(ProbCutTestAccess::Run(checked, condition));
  EXPECT_EQ(checked.GetInfo().probcut_tries, 0u);
}

TEST(ProbCut, TtVetoRespectsDepthThresholdScoreAndDecisiveBand) {
  const auto position = PositionFactory{}(kProbCutFen);
  DepthCondition condition{8};
  struct Case {
    Depth depth;
    Eval value;
    bool veto;
  };
  for (const auto test :
       {Case{1, -1, false}, Case{2, -1, true}, Case{2, 0, false},
        Case{2, 1, false}, Case{2, kTablebaseWinValue - 10, false},
        Case{2, -kTablebaseWinValue + 10, false},
        Case{2, kMateValue + 10, false}}) {
    for (const auto bound : {Bound::kLower, Bound::kUpper, Bound::kExact}) {
      auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
      table->SetEntry(position, Move::None(), ToTTScore(test.value, 1),
                      test.depth, bound, 0);
      Searcher searcher(position, table);
      ProbCutTestAccess::Run(searcher, condition);
      EXPECT_EQ(searcher.GetInfo().probcut_tries, test.veto ? 0u : 1u);
      EXPECT_EQ(table->GetNode(position).score, ToTTScore(test.value, 1));
      ExpectProbCutPositionRestored(searcher.GetPosition(), position);
    }
  }
}

TEST(ProbCut, SeeBoundaryUsesImprovingMarginInLocalUnits) {
  const auto position = PositionFactory{}(kProbCutFen);
  const auto move = MoveFactory{}(position, "d4d5");
  const auto pawn = kPieceValues[static_cast<size_t>(Piece::kPawn)];
  const auto rook = kPieceValues[static_cast<size_t>(Piece::kRook)];
  const Eval gain = pawn(rook.eval[0]);
  ASSERT_TRUE(position.StaticExchangeEvaluation(move, gain));
  ASSERT_FALSE(position.StaticExchangeEvaluation(move, gain + 1));
  DepthCondition condition{8};
  for (const bool improving : {false, true}) {
    const Eval margin = improving ? 70 : 95;
    for (const Eval extra : {0, 1}) {
      Searcher searcher(position);
      const Eval beta = position.Evaluate() + gain + extra - margin;
      ProbCutTestAccess::Run(searcher, condition, {8, 5, beta - 1, beta, 1},
                             improving);
      EXPECT_EQ(searcher.GetInfo().probcut_candidates, extra == 0 ? 1u : 0u);
      ExpectProbCutPositionRestored(searcher.GetPosition(), position);
    }
  }
}

TEST(ProbCut, TacticalPickerOrdersCapturesAndOmitsQuietsAndUnderpromotions) {
  auto position = PositionFactory{}("7k/P7/8/3q4/3R3p/8/8/K7 w - - 0 1");
  Searcher searcher(position);
  MovePicker picker;
  picker.InitProbCut(position, &searcher, 1);
  std::vector<Move> moves;
  for (auto move = picker.SelectProbCutMove(-2000); move.IsValid();
       move = picker.SelectProbCutMove(-2000)) {
    moves.push_back(move);
    EXPECT_FALSE(move.IsQuiet(position));
    EXPECT_TRUE(!move.IsPromotion() || move.PromotionPiece() == Piece::kQueen);
  }
  ASSERT_EQ(moves.size(), 3u);
  EXPECT_EQ(moves[0], MoveFactory{}(position, "d4d5"));
  EXPECT_EQ(moves[1], MoveFactory{}(position, "d4h4"));
  EXPECT_EQ(moves[2], MoveFactory{}(position, "a7a8q"));
}

TEST(ProbCut, TacticalPickerIncludesEnPassantAndRejectsIllegalPinnedCapture) {
  auto position = PositionFactory{}("k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
  Searcher searcher(position);
  MovePicker picker;
  picker.InitProbCut(position, &searcher, 1);
  EXPECT_FALSE(picker.SelectProbCutMove(-2000).IsValid());
  position = PositionFactory{}("k7/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
  picker.InitProbCut(position, &searcher, 1);
  EXPECT_EQ(picker.SelectProbCutMove(0), MoveFactory{}(position, "e5d6"));
  EXPECT_FALSE(picker.SelectProbCutMove(0).IsValid());
}

TEST(ProbCut, CancelledProbeRestoresPositionAndSuppressesCutoff) {
  const auto position = PositionFactory{}(kProbCutFen);
  auto stop = std::make_shared<std::atomic_bool>(true);
  DepthCondition condition{8, stop};
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  Searcher searcher(position, table);
  const auto result = ProbCutTestAccess::Run(searcher, condition);
  ASSERT_TRUE(result);
  EXPECT_FALSE(*result);
  EXPECT_EQ(searcher.GetInfo().probcut_candidates, 1u);
  EXPECT_EQ(searcher.GetInfo().probcut_qsearch_nodes, 1u);
  EXPECT_EQ(searcher.GetInfo().quiescence_nodes, 1u);
  EXPECT_EQ(searcher.GetInfo().probcut_qsearch_passes, 0u);
  EXPECT_EQ(searcher.GetInfo().probcut_cuts, 0u);
  EXPECT_FALSE(table->GetNode(position).IsOccupied());
  ExpectProbCutPositionRestored(searcher.GetPosition(), position);
}

TEST(ProbCut, MainSearchUsesVerifiedCutoffBeforeOrdinaryMoveLoop) {
  const auto position = PositionFactory{}(kProbCutFen);
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  Searcher searcher(position, table);
  DepthCondition condition{8};
  const auto result = SearchNode<NodeType::kAll, DepthCondition>{
      searcher, {8, 6, -96, -95, 1}, condition}();
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, -95);
  EXPECT_EQ(searcher.GetInfo().probcut_tries, 1u);
  EXPECT_EQ(searcher.GetInfo().probcut_cuts, 1u);
  EXPECT_FALSE(table->GetNode(position).IsOccupied());
  ExpectProbCutPositionRestored(searcher.GetPosition(), position);
}
