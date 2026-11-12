#include <gtest/gtest.h>

#include <ranges>
#include <utility>

#include "InplaceVector.h"

using namespace SimpleChessEngine;

static_assert(std::ranges::contiguous_range<InplaceVector<int, 256>>);
static_assert(std::ranges::contiguous_range<const InplaceVector<int, 256>>);
static_assert([] {
  InplaceVector<int, 256> values;
  for (int value = 0; value < 256; ++value) values.unchecked_push_back(value);
  if (values.size() != values.capacity() || values.back() != 255) return false;
  const auto removed = erase_if(values, [](int value) { return value % 2 == 0; });
  return removed == 128 && values.size() == 128 && values.front() == 1 &&
         std::as_const(values).back() == 255;
}());

TEST(InplaceVector, BackRefersToLastElementAfterErasure) {
  InplaceVector<int, 3> values;
  values.unchecked_push_back(1);
  values.unchecked_push_back(2);
  values.unchecked_push_back(3);
  values.back() = 4;
  EXPECT_EQ(erase_if(values, [](int value) { return value == 4; }), 1);
  EXPECT_EQ(std::as_const(values).back(), 2);
  EXPECT_EQ(values.size(), 2);
}

static_assert([] {
  InplaceVector<int, 3> values;
  values.unchecked_push_back(1);
  values.unchecked_push_back(2);
  values.unchecked_push_front(values.back());
  return values.size() == 3 && values.front() == 2 && values[1] == 1 &&
         values.back() == 2;
}());

TEST(InplaceVector, PushFrontPreservesAliasedValue) {
  InplaceVector<int, 3> values;
  values.unchecked_push_back(1);
  values.unchecked_push_back(2);
  values.unchecked_push_front(values.back());
  EXPECT_EQ(values.front(), 2);
  EXPECT_EQ(values[1], 1);
  EXPECT_EQ(values.back(), 2);
}
