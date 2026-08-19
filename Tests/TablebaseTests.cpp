#include <gtest/gtest.h>

#include "SyzygyTablebase.h"

using namespace SimpleChessEngine;

namespace TablebaseTests {

TEST(Syzygy, MissingDirectoryIsUnavailable) {
  EXPECT_FALSE(Tablebase::Syzygy::Open("/missing/syzygy"));
  Tablebase::Syzygy::Disable();
}

TEST(Wdl, ValuesMatchFathomOrdering) {
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kLoss), -2);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kBlessedLoss), -1);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kDraw), 0);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kCursedWin), 1);
  EXPECT_EQ(static_cast<int>(Tablebase::Wdl::kWin), 2);
}

}  // namespace TablebaseTests
