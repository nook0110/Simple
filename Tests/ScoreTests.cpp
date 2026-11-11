#include <gtest/gtest.h>

#include "Score.h"

using namespace SimpleChessEngine;

static_assert(SfMargin::kEnginePawn == 82);
static_assert(Eval{41} >= SfMargin{104});
static_assert(Eval{40} < SfMargin{104});
static_assert(Eval{-41} <= SfMargin{-104});
static_assert(Eval{-40} > SfMargin{-104});
static_assert(Eval{-788} >= SfMargin{-2000});
static_assert(Eval{-789} < SfMargin{-2000});
static_assert(SfMargin{365} - SfMargin{13} * 2 == SfMargin{339});
static_assert(SfMargin::RoundedFromEngine(41) == 104);
static_assert(SfMargin::RoundedFromEngine(-41) == -104);
static_assert(SfMargin::RoundedFromEngine(1) == 3);
static_assert(SfMargin{208}.ToEngine() == 82.0);

TEST(SfMargin, MatchesTheEngineValuesItReplaced) {
  EXPECT_EQ(SfMargin{5}.ToEngine(), 5.0 * 82 / 208.0);
  EXPECT_EQ(SfMargin{1}.ToEngine(), 82 / 208.0);
  EXPECT_EQ(SfMargin{10193}.ToEngine(), 10193.0 * 82 / 208.0);
  EXPECT_EQ(SfMargin::kPerEngineUnit, 208.0 / 82);
}
