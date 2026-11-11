#include <gtest/gtest.h>

#include "PositionFactory.h"
#include "SyzygyTablebase.h"

using namespace SimpleChessEngine;

namespace TablebaseTests {

TEST(Syzygy, MissingDirectoryIsUnavailable) {
  EXPECT_FALSE(Tablebase::Syzygy::Open("/missing/syzygy"));
  Tablebase::Syzygy::Disable();
}

TEST(Syzygy, RejectsIllegalKings) {
  const std::filesystem::path path{
      "/home/blokhtin/github/lichess-bot/engines/syzygy"};
  if (!std::filesystem::is_directory(path)) {
    GTEST_SKIP() << "Syzygy tablebase directory is unavailable";
  }
  const auto tablebase = Tablebase::Syzygy::Open(path);
  ASSERT_TRUE(tablebase);
  struct TablebaseGuard {
    ~TablebaseGuard() { Tablebase::Syzygy::Disable(); }
  } guard;
  for (const auto* fen :
       {"8/8/8/4k3/8/8/2K5/4R3 w - - 0 1", "8/8/8/4k3/8/8/2K5/4Q3 w - - 0 1",
        "8/8/8/4k3/8/8/2K5/4R2R w - - 0 1", "8/8/8/8/8/8/2K5/4R3 b - - 0 1"}) {
    const auto position = PositionFactory{}(fen);
    EXPECT_FALSE(tablebase->RootMove(position)) << fen;
    EXPECT_FALSE(tablebase->Probe(position)) << fen;
    EXPECT_FALSE(tablebase->ProbeWdl(position)) << fen;
  }
  const auto position = PositionFactory{}("8/8/8/4k3/8/8/2K5/R7 w - - 0 1");
  EXPECT_TRUE(tablebase->RootMove(position));
  EXPECT_EQ(tablebase->ProbeWdl(position), Tablebase::Wdl::kWin);
}

TEST(Wdl, ValuesMatchFathomOrdering) {
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kLoss), -2);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kBlessedLoss), -1);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kDraw), 0);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kCursedWin), 1);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kWin), 2);
}

}  // namespace TablebaseTests
