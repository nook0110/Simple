#include <gtest/gtest.h>

#include <array>
#include <ranges>

#include "BitBoard.h"

static_assert(sizeof(Bitboard) == sizeof(std::uint64_t));
static_assert(std::ranges::forward_range<Bitboard>);
static_assert(std::ranges::forward_range<const Bitboard>);
static_assert([] {
  Bitboard board;
  board.Set(0).Set(27).Set(63).Flip(27);
  if (board.Count() != 2 || !board.MoreThanOne()) return false;
  std::array<BitIndex, 2> squares{};
  size_t index = 0;
  for (const auto square : board) squares[index++] = square;
  if (squares != std::array<BitIndex, 2>{0, 63}) return false;
  if (board.PopFirstBit() != 0 || board.GetFirstBit() != 63) return false;
  board.Reset(63).Set().Reset();
  return board.None();
}());

TEST(BitboardRange, MatchesPopFirstBitWithoutChangingBoard) {
  for (const auto value :
       {0ull, 1ull, 0x8000000000000000ull, 0xf00d0123456789abull, ~0ull}) {
    const Bitboard board{value};
    auto remaining = board;
    size_t count = 0;
    for (const auto square : board) {
      ASSERT_TRUE(remaining.Any());
      EXPECT_EQ(square, remaining.PopFirstBit());
      ++count;
    }
    EXPECT_TRUE(remaining.None());
    EXPECT_EQ(count, board.Count());
    EXPECT_EQ(static_cast<std::uint64_t>(board), value);
  }
}
