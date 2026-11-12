#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "EnumArray.h"
#include "Evaluation.h"
#include "Searcher.h"

using namespace SimpleChessEngine;

template <class Table, class Key>
concept IndexedBy = requires(Table table, Key key) { table[key]; };

static_assert(IndexedBy<ByPlayer<int>, Player>);
static_assert(!IndexedBy<ByPlayer<int>, Piece>);
static_assert(!IndexedBy<ByPlayer<int>, std::size_t>);
static_assert(IndexedBy<ByPiece<int>, Piece>);
static_assert(!IndexedBy<ByPiece<int>, Player>);
static_assert(!IndexedBy<ByCastlingSide<int>, std::size_t>);
static_assert(sizeof(ByPlayer<int>) == sizeof(std::array<int, kColors>));
static_assert(sizeof(ByPiece<int>) == sizeof(std::array<int, kPieceTypes>));
using ContinuationHistory = std::remove_cvref_t<
    decltype(std::declval<const Searcher&>().GetContinuationHistory())>;
static_assert(IndexedBy<ContinuationHistory, Piece>);
static_assert(!IndexedBy<ContinuationHistory, Player>);
static_assert(sizeof(ContinuationHistory) ==
              sizeof(std::int32_t) * kPieceTypes * kPieceTypes *
                  (kBoardArea + 1) * (kBoardArea + 1));
static_assert([] {
  ByPlayer<ByPiece<int>> table{};
  table[Player::kWhite][Piece::kPawn] = 7;
  table[Player::kBlack][Piece::kQueen] = -3;
  return table[Player::kWhite][Piece::kPawn] == 7 &&
         table[Player::kBlack][Piece::kQueen] == -3 &&
         table[Player::kWhite][Piece::kQueen] == 0;
}());

TEST(EnumArray, FillPreservesPlayerAndPieceMapping) {
  ByPlayer<ByPiece<int>> table{};
  table[Player::kWhite].fill(11);
  table[Player::kBlack].fill(-5);
  EXPECT_EQ(table[Player::kWhite][Piece::kKnight], 11);
  EXPECT_EQ(table[Player::kBlack][Piece::kKnight], -5);
}
