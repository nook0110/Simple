#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "PositionFactory.h"
#include "TranspositionTable.h"

namespace SimpleChessEngine::TranspositionTableTests {
TEST(SharedClusterTable, ConcurrentEntriesStayConsistent) {
  using Table = TranspositionTable<640>;
  using MaxTable = TranspositionTable<256 * 1024>;
  static_assert(Table::kClusterSize == 4);
  static_assert(Table::kEntryCount == 40ULL * 1024 * 1024);
  static_assert(Table::kMemoryBytes == 640ULL * 1024 * 1024);
  static_assert(MaxTable::kMemoryBytes == 256ULL * 1024 * 1024 * 1024);

  Table table;
  const Position position = PositionFactory{}();
  constexpr std::array moves = {Move{12, 28}, Move{11, 27}, Move{6, 21},
                                Move{1, 18}};
  std::atomic_bool done = false;

  std::thread reader([&] {
    while (!done.load(std::memory_order_relaxed)) {
      const Node node = table.GetNode(position);
      if (!node.IsOccupied()) continue;
      ASSERT_EQ(node.true_hash, position.GetHash());
      ASSERT_GE(node.score, 0);
      ASSERT_LT(node.score, static_cast<Eval>(moves.size()));
      EXPECT_EQ(node.move, moves[static_cast<std::size_t>(node.score)]);
    }
  });

  std::vector<std::thread> writers;
  for (std::size_t worker = 0; worker < moves.size(); ++worker) {
    writers.emplace_back([&, worker] {
      for (int iteration = 0; iteration < 20'000; ++iteration) {
        table.SetEntry(position, moves[worker], static_cast<Eval>(worker), 12,
                       Bound::kExact, static_cast<Age>(iteration));
      }
    });
  }
  for (auto& writer : writers) writer.join();
  done.store(true, std::memory_order_relaxed);
  reader.join();
}

TEST(SharedClusterTable, CachedEvalPayloadRoundTrips) {
  using Table = TranspositionTable<1>;
  constexpr Hash hash = 0x123456789ABCDEF0ULL;
  constexpr std::array scores = {
      -131072, kMateValue - 64, kMateValue, -kTablebaseWinValue, -20000, -1, 0,
      1, 20000, kTablebaseWinValue, -kMateValue, -kMateValue + 64, 131071};
  for (const auto score : scores) {
    for (const auto eval : {kNoStaticEval, -8191, -1, 0, 1, 8191}) {
      const Node original{hash, Move{12, 28}, score, 63, Bound::kExact, 255,
                          eval};
      const auto unpacked = Table::Unpack(hash, Table::Pack(original));
      EXPECT_EQ(unpacked.true_hash, original.true_hash);
      EXPECT_EQ(unpacked.move, original.move);
      EXPECT_EQ(unpacked.score, original.score);
      EXPECT_EQ(unpacked.depth, original.depth);
      EXPECT_EQ(unpacked.bound, original.bound);
      EXPECT_EQ(unpacked.age, original.age);
      EXPECT_EQ(unpacked.static_eval, original.static_eval);
    }
  }
  for (const auto eval : {-20000, -8193, 8192, 20000}) {
    const Node original{hash, Move{12, 28}, 17, 12, Bound::kLower, 7, eval};
    const auto unpacked = Table::Unpack(hash, Table::Pack(original));
    EXPECT_EQ(unpacked.score, original.score);
    EXPECT_EQ(unpacked.static_eval, kNoStaticEval);
  }
}

TEST(SharedClusterTable, CachedEvalRejectsDifferentContext) {
  TranspositionTable<1> table;
  const Position position = PositionFactory{}();
  constexpr Move move{12, 28};
  table.SetEntry(position, move, 321, 12, Bound::kExact, 1, -57, 11);
  const auto node = table.GetNode(position);
  EXPECT_EQ(table.GetStaticEval(node, 11), -57);
  EXPECT_EQ(table.GetStaticEval(node, 12), kNoStaticEval);
  table.SetEntry(position, move, 432, 13, Bound::kExact, 1, 100, 12);
  const auto changed = table.GetNode(position);
  EXPECT_EQ(changed.score, 432);
  EXPECT_EQ(table.GetStaticEval(changed, 12), kNoStaticEval);
  EXPECT_EQ(table.GetStaticEval(changed, 11), kNoStaticEval);
  table.SetEntry(position, move, 543, 14, Bound::kExact, 1, 7, 11);
  EXPECT_EQ(table.GetStaticEval(table.GetNode(position), 11), 7);
}
}  // namespace SimpleChessEngine::TranspositionTableTests
