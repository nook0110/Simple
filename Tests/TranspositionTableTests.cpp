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
}  // namespace SimpleChessEngine::TranspositionTableTests
