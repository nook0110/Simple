#include <gtest/gtest.h>

#include <functional>
#include <memory>

#include "CheckedMove.h"
#include "ExitCondition.h"
#include "MoveFactory.h"
#include "PositionFactory.h"
#include "Searcher.h"
#include "Settings.h"
#include "StaticEvalHistory.h"

using namespace SimpleChessEngine;

TEST(StaticEvalHistory, MissingAncestorsDoNotSetTrends) {
  StaticEvalHistory history;
  const auto root = history.Update(0, 50);
  EXPECT_FALSE(root.improving);
  EXPECT_FALSE(root.opponent_worsening);

  history.Clear();
  const auto child = history.Update(1, 50);
  EXPECT_FALSE(child.improving);
  EXPECT_FALSE(child.opponent_worsening);
}

TEST(StaticEvalHistory, ComparesSameSideAndOppositeSideEvaluations) {
  StaticEvalHistory history;
  const auto root = history.Update(0, -20);
  const auto child = history.Update(1, 10);
  EXPECT_FALSE(root.improving);
  EXPECT_FALSE(child.opponent_worsening);

  const auto improving = history.Update(2, -19);
  EXPECT_TRUE(improving.improving);
  EXPECT_FALSE(improving.opponent_worsening);

  const auto worsening = history.Update(2, -21);
  EXPECT_FALSE(worsening.improving);
  EXPECT_FALSE(worsening.opponent_worsening);

  const auto both = history.Update(2, -9);
  EXPECT_TRUE(both.improving);
  EXPECT_TRUE(both.opponent_worsening);
}

TEST(StaticEvalHistory, EqualEvaluationsDoNotSetTrends) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(0, 20).improving);
  EXPECT_FALSE(history.Update(1, -20).opponent_worsening);
  const auto equal = history.Update(2, 20);
  EXPECT_FALSE(equal.improving);
  EXPECT_FALSE(equal.opponent_worsening);
}

TEST(StaticEvalHistory, CheckClearsSiblingEvaluationAndFallsBackFourPlies) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(0, 20).improving);
  EXPECT_TRUE(history.Update(2, 100).improving);
  const auto checked = history.Update(2, std::nullopt);
  EXPECT_FALSE(checked.improving);
  EXPECT_FALSE(checked.opponent_worsening);

  const auto fallback = history.Update(4, 21);
  EXPECT_TRUE(fallback.improving);
  EXPECT_FALSE(fallback.opponent_worsening);
  EXPECT_FALSE(history.Update(4, 20).improving);
}

TEST(StaticEvalHistory, MissingFallbackDoesNotSetImproving) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(4, 500).improving);
  EXPECT_FALSE(history.Update(3, std::nullopt).opponent_worsening);
  EXPECT_FALSE(history.Update(4, 500).opponent_worsening);
}

TEST(StaticEvalHistory, ResetDiscardsPreviousSearchEvaluations) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(0, -20).improving);
  EXPECT_TRUE(history.Update(1, 21).opponent_worsening);
  history.Clear();
  const auto after_reset = history.Update(2, 50);
  EXPECT_FALSE(after_reset.improving);
  EXPECT_FALSE(after_reset.opponent_worsening);
}

TEST(StaticEvalHistory, SupportsMaximumPly) {
  StaticEvalHistory history;
  EXPECT_FALSE(history.Update(kMaxSearchPly - 2, 20).improving);
  EXPECT_FALSE(history.Update(kMaxSearchPly - 1, -21).opponent_worsening);
  const auto last = history.Update(kMaxSearchPly, 22);
  EXPECT_TRUE(last.improving);
  EXPECT_TRUE(last.opponent_worsening);
}

using RFP = Settings::PruneParameters::RFPSettings;
static_assert(RFP{}.Margin(5, false, false, false) == 500);
static_assert(RFP{}.Margin(5, false, true, false) == 300);
static_assert(RFP{}.Margin(5, false, false, true) == 467);
static_assert(RFP{}.Margin(5, false, true, true) == 267);
static_assert(RFP{}.Margin(5, true, false, false) == 385);
static_assert(RFP{}.Margin(5, true, true, false) == 231);
static_assert(RFP{}.Margin(5, true, false, true) == 360);
static_assert(RFP{}.Margin(5, true, true, true) == 206);
static_assert(RFP{}.Margin(1, false, true, true) == -133);

using LMP = Settings::PruneParameters::LMPSettings;
static_assert(LMP{}.MoveCount(1, false) == 2);
static_assert(LMP{}.MoveCount(1, true) == 4);
static_assert(LMP{}.MoveCount(2, false) == 3);
static_assert(LMP{}.MoveCount(2, true) == 7);
static_assert(LMP{}.MoveCount(8, false) == 33);
static_assert(LMP{}.MoveCount(8, true) == 67);

using LMR = Settings::PruneParameters::LMRSettings;
static_assert(LMR::ScaleReduction(4.0, true) == 4.0);
static_assert(LMR::ScaleReduction(4.0, false) == 5.4921875);
static_assert(LMR::ScaleReduction(0.0, false) == 0.0);

TEST(EvaluationTrendPruning, ReverseFutilityRespectsQuietTtMove) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  DepthCondition condition{3};
  for (const bool quiet_tt_move : {false, true}) {
    auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
    if (quiet_tt_move) {
      table->SetEntry(position, move, ToTT(0, 0), 0, Bound::kUpper, 0);
    }
    Searcher searcher(position, table);
    const auto result = SearchNode<NodeType::kAll, DepthCondition>{
        searcher, {3, 3, -1001, -1000, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(searcher.GetInfo().searched_nodes == 1, !quiet_tt_move);
  }
}

TEST(EvaluationTrendPruning, ReverseFutilityDoesNotCutLosingTablebaseBounds) {
  const auto position = PositionFactory{}();
  DepthCondition condition{2};
  const Eval loss_bound = -Score::kDecisive;
  for (const Eval beta : {loss_bound, loss_bound + 1}) {
    Searcher searcher(position);
    const auto result = SearchNode<NodeType::kAll, DepthCondition>{
        searcher, {2, 2, beta - 1, beta, 1}, condition}();
    ASSERT_TRUE(result);
    EXPECT_EQ(searcher.GetInfo().searched_nodes == 1, beta > loss_bound);
  }
}

namespace SimpleChessEngine {
struct SearchEvaluationTestAccess {
  static constexpr auto Bonus(Eval parent, Eval child) {
    return Searcher::StaticEvalHistoryBonus(parent, child);
  }
  static void Play(Searcher &searcher, Depth ply, const Move &move,
                   std::optional<Eval> parent_eval) {
    searcher.SetPlayedMove(ply, move, parent_eval);
  }
  static void Null(Searcher &searcher, Depth ply) {
    searcher.SetPlayedNullMove(ply);
  }
  static void Train(Searcher &searcher, Depth ply, Eval eval) {
    searcher.UpdateStaticEvalHistory(ply, eval);
  }
  static auto &Entry(Searcher &searcher, Player side, const Move &move) {
    return searcher.history_[side][move.From()][move.To()];
  }
  static bool ContinuationsEmpty(const Searcher &searcher) {
    const auto entries = searcher.GetContinuationHistory() | std::views::join |
                         std::views::join | std::views::join;
    return std::ranges::all_of(entries, [](auto entry) { return entry == 0; });
  }
  static const auto &Context(const Searcher &searcher, Depth ply) {
    return searcher.move_stack_[ply];
  }
  template <NodeType type, class Condition>
  static bool HasStaticEval(const SearchNode<type, Condition> &node) {
    return node.position_info_.static_eval.has_value();
  }
};
}  // namespace SimpleChessEngine

namespace {
using PolicyAccess = SearchEvaluationTestAccess;

struct PolicyStopCondition {
  std::function<bool()> stop;
  bool IsTimeToExit() const { return stop(); }
};

}  // namespace

namespace SimpleChessEngine {
template <>
Quiescence<PolicyStopCondition>::Quiescence(
    const PolicyStopCondition &condition, Table *table, Age age)
    : exit_condition_(condition), table_(table), age_(age),
      cache_static_eval_(false), eval_context_(0) {}

template <>
template <>
SearchResult Quiescence<PolicyStopCondition>::Search<true>(Position &, Eval,
                                                           Eval, Depth, bool) {
  ADD_FAILURE() << "Policy probe unexpectedly reached quiescence";
  return std::nullopt;
}
}  // namespace SimpleChessEngine

namespace {
void ProbePolicyNode(Searcher &searcher, Depth ply = 1,
                     std::optional<Move> excluded = std::nullopt) {
  const auto initial_nodes = searcher.GetInfo().searched_nodes;
  const PolicyStopCondition stop{
      [&] { return searcher.GetInfo().searched_nodes > initial_nodes; }};
  EXPECT_FALSE((SearchNode<NodeType::kPV, PolicyStopCondition>{
      searcher, {6, 3, -10000, 10000, ply}, stop, excluded}()));
}
}  // namespace

static_assert(PolicyAccess::Bonus(20, -40) == 2785);
static_assert(PolicyAccess::Bonus(20, 0) == 226);
static_assert(PolicyAccess::Bonus(20, -20) == 1505);
static_assert(PolicyAccess::Bonus(-20, 0) == 2785);
static_assert(PolicyAccess::Bonus(0, 0) == 1505);
static_assert(PolicyAccess::Bonus(kTempoBonus, kTempoBonus) == -1029);
static_assert(PolicyAccess::Bonus(0, -1) == 1581);
static_assert(PolicyAccess::Bonus(0, 1) == 1430);

static_assert(PolicyAccess::Bonus(0, 74) == -3212);
static_assert(PolicyAccess::Bonus(0, 75) == -3237);
static_assert(PolicyAccess::Bonus(0, 10000) == -3237);
static_assert(PolicyAccess::Bonus(0, -76) == 6348);
static_assert(PolicyAccess::Bonus(0, -77) == 6373);
static_assert(PolicyAccess::Bonus(0, -10000) == 6373);

TEST(StaticEvalPolicy, UpdatesSavedMoverOnlyAndLeavesContinuationUnchanged) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  auto child = position;
  child.DoMove(CheckedMove(child, move));
  Searcher searcher(position);
  PolicyAccess::Play(searcher, 0, move, 20);
  PolicyAccess::Play(searcher, 1, move, 20);
  searcher.SetPosition(child);
  PolicyAccess::Train(searcher, 2, -40);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, move), 2785);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kBlack, move), 0);
  EXPECT_TRUE(PolicyAccess::ContinuationsEmpty(searcher));
}

TEST(StaticEvalPolicy, GravityStaysBoundedAndUsesExistingDecay) {
  const auto position = PositionFactory{}();
  const auto move = MoveFactory{}(position, "e2e4");
  Searcher searcher(position);
  PolicyAccess::Play(searcher, 0, move, 0);
  auto &entry = PolicyAccess::Entry(searcher, Player::kWhite, move);
  entry = 8000;
  PolicyAccess::Train(searcher, 1, -20);
  EXPECT_EQ(entry, 8000 + 2785 - 8000 * 2785 / 16384);
  for (const Eval child : {-10000, 10000}) {
    for (int i = 0; i < 1000; ++i) {
      PolicyAccess::Train(searcher, 1, child);
      ASSERT_LE(entry, 16384);
      ASSERT_GE(entry, -16384);
    }
  }
}

TEST(StaticEvalPolicy, SiblingAndNullReplacePreviousMoveContext) {
  const auto position = PositionFactory{}();
  const auto first = MoveFactory{}(position, "e2e4");
  const auto second = MoveFactory{}(position, "d2d4");
  Searcher searcher(position);
  PolicyAccess::Play(searcher, 0, first, 20);
  PolicyAccess::Play(searcher, 0, second, -20);
  PolicyAccess::Train(searcher, 1, 0);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, first), 0);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, second), 2785);
  PolicyAccess::Null(searcher, 0);
  PolicyAccess::Train(searcher, 1, -10000);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, second), 2785);
  searcher.InitStartOfSearch();
  const auto decayed = PolicyAccess::Entry(searcher, Player::kWhite, second);
  PolicyAccess::Train(searcher, 1, -10000);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, second), decayed);
}

TEST(StaticEvalPolicy, ChildTrainsBeforeCutoffAndRestoresBoard) {
  const auto parent = PositionFactory{}();
  const auto move = MoveFactory{}(parent, "e2e4");
  auto child = parent;
  child.DoMove(CheckedMove(child, move));
  const auto parent_eval = parent.Evaluate();
  const auto child_eval = child.Evaluate();
  Searcher searcher(parent);
  PolicyAccess::Play(searcher, 0, move, parent_eval);
  searcher.SetPosition(child);
  ProbePolicyNode(searcher);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, move),
            PolicyAccess::Bonus(parent_eval, child_eval));
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kBlack, move), 0);
  EXPECT_TRUE(PolicyAccess::ContinuationsEmpty(searcher));
  EXPECT_EQ(searcher.GetPosition().GetHash(), child.GetHash());
  EXPECT_EQ(searcher.GetPosition().Evaluate(), child_eval);
  EXPECT_EQ(searcher.GetPosition().GetIrreversibleData(),
            child.GetIrreversibleData());
}

TEST(StaticEvalPolicy, SuppressesNullMissingParentRootAndExclusion) {
  const auto parent = PositionFactory{}();
  const auto move = MoveFactory{}(parent, "e2e4");
  auto child = parent;
  child.DoMove(CheckedMove(child, move));
  for (int test = 0; test < 4; ++test) {
    SCOPED_TRACE(test);
    Searcher searcher(parent);
    PolicyAccess::Play(searcher, 0, move,
                       test == 0 ? std::nullopt : std::optional{20});
    if (test == 1) PolicyAccess::Null(searcher, 0);
    searcher.SetPosition(child);
    ProbePolicyNode(
        searcher, test == 2 ? 0 : 1,
        test == 3 ? std::optional{MoveFactory{}(child, "e7e5")} : std::nullopt);
    EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, move), 0);
  }
}

TEST(StaticEvalPolicy,
     SuppressesCapturesPromotionsEnPassantAndCheckedChildren) {
  struct Case {
    const char *fen;
    const char *move;
  };
  for (const auto test : {Case{"4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1", "e4d5"},
                          Case{"8/P3k3/8/8/8/8/8/4K3 w - - 0 1", "a7a8q"},
                          Case{"4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6"},
                          Case{"4k3/8/8/8/8/8/8/R3K3 w - - 0 1", "a1a8"}}) {
    SCOPED_TRACE(test.move);
    const auto parent = PositionFactory{}(test.fen);
    const auto move = MoveFactory{}(parent, test.move);
    Searcher searcher(parent);
    PolicyAccess::Play(searcher, 0, move, parent.Evaluate());
    auto child = parent;
    child.DoMove(CheckedMove(child, move));
    searcher.SetPosition(child);
    ProbePolicyNode(searcher);
    EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, move), 0);
    EXPECT_EQ(searcher.GetPosition().GetHash(), child.GetHash());
  }
}

TEST(StaticEvalPolicy, SearchSavesRawParentBeforeMakingMove) {
  const auto parent = PositionFactory{}();
  const auto parent_eval = parent.Evaluate();
  Searcher searcher(parent);
  bool observed = false;
  const PolicyStopCondition stop{[&] {
    if (searcher.GetInfo().searched_nodes == 0) return false;
    observed = true;
    const auto &context = PolicyAccess::Context(searcher, 0);
    EXPECT_TRUE(context.move.IsValid());
    EXPECT_EQ(context.side, Player::kWhite);
    EXPECT_TRUE(context.quiet);
    EXPECT_EQ(context.parent_eval, parent_eval);
    EXPECT_NE(searcher.GetPosition().GetHash(), parent.GetHash());
    return true;
  }};
  EXPECT_FALSE((SearchNode<NodeType::kPV, PolicyStopCondition>{
      searcher, {6, 3, -10000, 10000}, stop}()));
  EXPECT_TRUE(observed);
  EXPECT_EQ(searcher.GetPosition().GetHash(), parent.GetHash());
}

TEST(StaticEvalPolicy, TtCutoffSkipsEvaluationAndPolicyTraining) {
  const auto parent = PositionFactory{}();
  const auto move = MoveFactory{}(parent, "e2e4");
  auto child = parent;
  child.DoMove(CheckedMove(child, move));
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  table->SetEntry(child, MoveFactory{}(child, "e7e5"), ToTT(-1, 0), 3,
                  Bound::kUpper, 0);
  Searcher searcher(parent, table);
  PolicyAccess::Play(searcher, 0, move, 20);
  searcher.SetPosition(child);
  DepthCondition stop{3};
  SearchNode<NodeType::kAll, DepthCondition> node{
      searcher, {3, 3, -1, 0, 1}, stop};
  const auto result = node();
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, -1);
  EXPECT_FALSE(PolicyAccess::HasStaticEval(node));
  EXPECT_EQ(searcher.GetInfo().searched_nodes, 1u);
  EXPECT_EQ(PolicyAccess::Entry(searcher, Player::kWhite, move), 0);
}

TEST(StaticEvalPolicy, CheckedAndExcludedParentsSaveNoEvaluation) {
  for (const bool checked : {false, true}) {
    const auto parent = PositionFactory{}(
        checked ? "4k3/8/8/8/8/8/4r3/4K1N1 w - - 0 1"
                : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Searcher searcher(parent);
    bool observed = false;
    const PolicyStopCondition stop{[&] {
      if (searcher.GetInfo().searched_nodes == 0) return false;
      observed = true;
      const auto &context = PolicyAccess::Context(searcher, 0);
      EXPECT_TRUE(context.move.IsValid());
      EXPECT_FALSE(context.parent_eval);
      return true;
    }};
    EXPECT_FALSE((SearchNode<NodeType::kPV, PolicyStopCondition>{
        searcher,
        {6, 3, -10000, 10000},
        stop,
        checked ? std::nullopt
                : std::optional{MoveFactory{}(parent, "e2e4")}}()));
    EXPECT_TRUE(observed);
    EXPECT_EQ(searcher.GetPosition().GetHash(), parent.GetHash());
  }
}

TEST(StaticEvalPolicy, NullSearchInvalidatesStaleQuietContext) {
  const auto parent = PositionFactory{}();
  Searcher searcher(parent);
  PolicyAccess::Play(searcher, 1, MoveFactory{}(parent, "e2e4"), 20);
  const auto beta = parent.Evaluate() - 200;
  bool observed = false;
  const PolicyStopCondition stop{[&] {
    if (searcher.GetInfo().nmp_tries == 0) return false;
    observed = true;
    const auto &context = PolicyAccess::Context(searcher, 1);
    EXPECT_FALSE(context.move.IsValid());
    EXPECT_FALSE(context.quiet);
    EXPECT_FALSE(context.parent_eval);
    return true;
  }};
  auto &rfp = Settings::current<RFP>;
  const auto rfp_limit = rfp.depth_limit;
  rfp.depth_limit = 0;
  EXPECT_FALSE((SearchNode<NodeType::kCut, PolicyStopCondition>{
      searcher, {12, 6, beta - 1, beta, 1}, stop}()));
  rfp.depth_limit = rfp_limit;
  EXPECT_TRUE(observed);
  EXPECT_EQ(searcher.GetPosition().GetHash(), parent.GetHash());
}

TEST(StaticEvalPolicy, SearchReplacesContextForLaterQuietSibling) {
  auto parent = PositionFactory{}();
  const auto parent_eval = parent.Evaluate();
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  for (const auto &move :
       MoveGenerator{}.GenerateLegalMoves<MoveGenerator::Type::kAll>(parent)) {
    auto child = parent;
    child.DoMove(move);
    table->SetEntry(child, MoveFactory{}(child, "e7e5"), ToTT(-9999, 0), 6,
                    Bound::kLower, 0);
  }
  Searcher searcher(parent, table);
  std::optional<Move> first;
  bool observed = false;
  const PolicyStopCondition stop{[&] {
    if (searcher.GetPosition().GetHash() == parent.GetHash()) return false;
    const auto &context = PolicyAccess::Context(searcher, 0);
    if (!first) {
      first = context.move;
      return false;
    }
    if (context.move == *first) return false;
    observed = true;
    EXPECT_TRUE(context.quiet);
    EXPECT_EQ(context.parent_eval, parent_eval);
    auto expected_child = parent;
    expected_child.DoMove(CheckedMove(expected_child, context.move));
    EXPECT_EQ(searcher.GetPosition().GetHash(), expected_child.GetHash());
    return true;
  }};
  EXPECT_FALSE((SearchNode<NodeType::kAll, PolicyStopCondition>{
      searcher, {6, 3, 9999, 10000}, stop}()));
  EXPECT_TRUE(observed);
  EXPECT_EQ(searcher.GetPosition().GetHash(), parent.GetHash());
}

TEST(SingularExtensionStatistics, AbortedVerificationIsNotATtCut) {
  const auto position = PositionFactory{}();
  auto table = std::make_shared<Searcher::SearcherTranspositionTable>();
  table->SetEntry(position, MoveFactory{}(position, "e2e4"), ToTT(10000, 0), 4,
                  Bound::kLower, 0);
  Searcher searcher(position, table);
  const PolicyStopCondition stop{
      [&] { return searcher.GetSingularInfo().searches > 0; }};
  EXPECT_FALSE((SearchNode<NodeType::kPV, PolicyStopCondition>{
      searcher, {8, 7, -20000, 20000, 1}, stop}()));
  EXPECT_EQ(searcher.GetSingularInfo().searches, 1u);
  EXPECT_EQ(searcher.GetInfo().tt_cuts, 0u);
  EXPECT_EQ(searcher.GetPosition().GetHash(), position.GetHash());
}
