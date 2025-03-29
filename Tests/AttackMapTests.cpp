#include <gtest/gtest.h>

#include "Chess/Attacks.h"
using namespace SimpleChessEngine;

namespace AttackMapTests {
struct TestCase {
  Bitboard occupancy;
  BitIndex square;
  Bitboard answer;
};

template <Piece sliding_piece>
class TestAttackMaskGeneration : public testing::TestWithParam<TestCase> {
 protected:
  [[nodiscard]] Bitboard GetMask() const {
    const auto [occupancy, square, answer] = GetParam();
    return GenerateAttackMask<sliding_piece>(square, occupancy);
  }
  [[nodiscard]] Bitboard GetAnswer() const { return GetParam().answer; }
};

using BishopMaskTest = TestAttackMaskGeneration<Piece::kBishop>;
using RookMaskTest = TestAttackMaskGeneration<Piece::kRook>;

TEST_P(BishopMaskTest, AttackMaskGeneration) {
  ASSERT_EQ(GetMask(), GetAnswer());
}

TEST_P(RookMaskTest, AttackMaskGeneration) {
  ASSERT_EQ(GetMask(), GetAnswer());
}

INSTANTIATE_TEST_CASE_P(
    RandomBoardBishop, BishopMaskTest,
    testing::Values(
        TestCase{Bitboard{0x40c601f030da9200}, 52,
                 Bitboard{0x2800284402010000}},
        TestCase{Bitboard{0x20346a0215120000}, 19, Bitboard{0x14001422}},
        TestCase{Bitboard{0x600120400c222288}, 53,
                 Bitboard{0x5000508804000000}},
        TestCase{Bitboard{0xc240904011582201}, 4, Bitboard{0x102042800}},
        TestCase{Bitboard{0x4c548408443a400}, 44, Bitboard{0x4428002844800000}},
        TestCase{Bitboard{0x1a529494440060f}, 17, Bitboard{0x5000500}},
        TestCase{Bitboard{0x1062002008502088}, 55,
                 Bitboard{0x4000402000000000}},
        TestCase{Bitboard{0x12420d18100c0642}, 12, Bitboard{0x8040280028}},
        TestCase{Bitboard{0x204c1808020c20a}, 52, Bitboard{0x2800284482010000}},
        TestCase{Bitboard{0xa0010290c120e}, 56, Bitboard{0x2000000000000}},
        TestCase{Bitboard{0xaa5450937910031}, 1, Bitboard{0x10080500}},
        TestCase{Bitboard{0x208a011a4424150}, 40, Bitboard{0x402000204000000}},
        TestCase{Bitboard{0x189a000194003820}, 56, Bitboard{0x2000000000000}},
        TestCase{Bitboard{0x40103129091431}, 17, Bitboard{0x100805000500}},
        TestCase{Bitboard{0x86140949401340a}, 43, Bitboard{0x2214001400000000}},
        TestCase{Bitboard{0x9c090a544c280040}, 42, Bitboard{0x10a000a11200000}},
        TestCase{Bitboard{0x21010a151a405350}, 44,
                 Bitboard{0x4428002844820100}},
        TestCase{Bitboard{0x4831b41488027311}, 3, Bitboard{0x21400}},
        TestCase{Bitboard{0x1012012370c1800}, 0, Bitboard{0x40200}},
        TestCase{Bitboard{0x1226004048909}, 52, Bitboard{0x2800280402010000}},
        TestCase{Bitboard{0x1213089000011c26}, 62, Bitboard{0xa0100804020100}},
        TestCase{Bitboard{0x88e4808200200c0c}, 49, Bitboard{0x500050810200000}},
        TestCase{Bitboard{0x40180e2241e00cc0}, 32, Bitboard{0x20002040800}},
        TestCase{Bitboard{0x8200c0002ca0c488}, 55,
                 Bitboard{0x4000400000000000}},
        TestCase{Bitboard{0x6600e00418318140}, 43,
                 Bitboard{0x2214001420408000}}));

INSTANTIATE_TEST_CASE_P(
    RandomBoardRook, RookMaskTest,
    testing::Values(
        TestCase{Bitboard{0x40c601f030da9200}, 52,
                 Bitboard{0x106c101000000000}},
        TestCase{Bitboard{0x20346a0215120000}, 19, Bitboard{0x80808160808}},
        TestCase{Bitboard{0x600120400c222288}, 53,
                 Bitboard{0x20df200000000000}},
        TestCase{Bitboard{0xc240904011582201}, 4, Bitboard{0x1010ef}},
        TestCase{Bitboard{0x4c548408443a400}, 44, Bitboard{0x1010681010101010}},
        TestCase{Bitboard{0x1a529494440060f}, 17, Bitboard{0x2020202027d0200}},
        TestCase{Bitboard{0x1062002008502088}, 55,
                 Bitboard{0x8040808080808080}},
        TestCase{Bitboard{0x12420d18100c0642}, 12, Bitboard{0x1010ec10}},
        TestCase{Bitboard{0x204c1808020c20a}, 52, Bitboard{0x10ec101010101010}},
        TestCase{Bitboard{0xa0010290c120e}, 56, Bitboard{0xfe01010101000000}},
        TestCase{Bitboard{0xaa5450937910031}, 1, Bitboard{0x202021d}},
        TestCase{Bitboard{0x208a011a4424150}, 40, Bitboard{0x1013e0100000000}},
        TestCase{Bitboard{0x189a000194003820}, 56, Bitboard{0xe01010100000000}},
        TestCase{Bitboard{0x40103129091431}, 17, Bitboard{0x2020202020d0202}},
        TestCase{Bitboard{0x86140949401340a}, 43, Bitboard{0x808770808080808}},
        TestCase{Bitboard{0x9c090a544c280040}, 42, Bitboard{0x4040a0400000000}},
        TestCase{Bitboard{0x21010a151a405350}, 44,
                 Bitboard{0x1010e81000000000}},
        TestCase{Bitboard{0x4831b41488027311}, 3, Bitboard{0x8080817}},
        TestCase{Bitboard{0x1012012370c1800}, 0, Bitboard{0x10101fe}},
        TestCase{Bitboard{0x1226004048909}, 52, Bitboard{0x10ef101010101010}},
        TestCase{Bitboard{0x1213089000011c26}, 62,
                 Bitboard{0xb040404040404040}},
        TestCase{Bitboard{0x88e4808200200c0c}, 49, Bitboard{0x205020200000000}},
        TestCase{Bitboard{0x40180e2241e00cc0}, 32, Bitboard{0x101010201000000}},
        TestCase{Bitboard{0x8200c0002ca0c488}, 55,
                 Bitboard{0x807f800000000000}},
        TestCase{Bitboard{0x6600e00418318140}, 43,
                 Bitboard{0x808370808000000}}));

struct TestCaseWithoutAnswer {
  Bitboard occupancy;
  BitIndex square;
};

template <Piece sliding_piece>
class TestAttackMapTable
    : public testing::TestWithParam<TestCaseWithoutAnswer> {
 protected:
  [[nodiscard]] Bitboard GetMask() const {
    auto test_case = GetParam();
    return AttackTable<sliding_piece>::GetAttackMap(test_case.square,
                                                    test_case.occupancy);
  }
  [[nodiscard]] Bitboard GetAnswer() const {
    const auto test_case = GetParam();
    return GenerateAttackMask<sliding_piece>(test_case.square,
                                             test_case.occupancy);
  }
};

using BishopAttackMapTest = TestAttackMapTable<Piece::kBishop>;
using RookAttackMapTest = TestAttackMapTable<Piece::kRook>;

TEST_P(BishopAttackMapTest, AttackMap) { ASSERT_EQ(GetMask(), GetAnswer()); }

TEST_P(RookAttackMapTest, AttackMap) { ASSERT_EQ(GetMask(), GetAnswer()); }

INSTANTIATE_TEST_CASE_P(
    RandomBoardBishop, BishopAttackMapTest,
    testing::Values(TestCaseWithoutAnswer{Bitboard{0x40c601f030da9200}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x20346a0215120000}, 19},
                    TestCaseWithoutAnswer{Bitboard{0x600120400c222288}, 53},
                    TestCaseWithoutAnswer{Bitboard{0xc240904011582201}, 4},
                    TestCaseWithoutAnswer{Bitboard{0x4c548408443a400}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x1a529494440060f}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x1062002008502088}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x12420d18100c0642}, 12},
                    TestCaseWithoutAnswer{Bitboard{0x204c1808020c20a}, 52},
                    TestCaseWithoutAnswer{Bitboard{0xa0010290c120e}, 56},
                    TestCaseWithoutAnswer{Bitboard{0xaa5450937910031}, 1},
                    TestCaseWithoutAnswer{Bitboard{0x208a011a4424150}, 40},
                    TestCaseWithoutAnswer{Bitboard{0x189a000194003820}, 56},
                    TestCaseWithoutAnswer{Bitboard{0x40103129091431}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x86140949401340a}, 43},
                    TestCaseWithoutAnswer{Bitboard{0x9c090a544c280040}, 42},
                    TestCaseWithoutAnswer{Bitboard{0x21010a151a405350}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x4831b41488027311}, 3},
                    TestCaseWithoutAnswer{Bitboard{0x1012012370c1800}, 0},
                    TestCaseWithoutAnswer{Bitboard{0x1226004048909}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x1213089000011c26}, 62},
                    TestCaseWithoutAnswer{Bitboard{0x88e4808200200c0c}, 49},
                    TestCaseWithoutAnswer{Bitboard{0x40180e2241e00cc0}, 32},
                    TestCaseWithoutAnswer{Bitboard{0x8200c0002ca0c488}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x6600e00418318140}, 43}));

INSTANTIATE_TEST_CASE_P(
    RandomBoardRook, RookAttackMapTest,
    testing::Values(TestCaseWithoutAnswer{Bitboard{0x40c601f030da9200}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x20346a0215120000}, 19},
                    TestCaseWithoutAnswer{Bitboard{0x600120400c222288}, 53},
                    TestCaseWithoutAnswer{Bitboard{0xc240904011582201}, 4},
                    TestCaseWithoutAnswer{Bitboard{0x4c548408443a400}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x1a529494440060f}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x1062002008502088}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x12420d18100c0642}, 12},
                    TestCaseWithoutAnswer{Bitboard{0x204c1808020c20a}, 52},
                    TestCaseWithoutAnswer{Bitboard{0xa0010290c120e}, 56},
                    TestCaseWithoutAnswer{Bitboard{0xaa5450937910031}, 1},
                    TestCaseWithoutAnswer{Bitboard{0x208a011a4424150}, 40},
                    TestCaseWithoutAnswer{Bitboard{0x189a000194003820}, 56},
                    TestCaseWithoutAnswer{Bitboard{0x40103129091431}, 17},
                    TestCaseWithoutAnswer{Bitboard{0x86140949401340a}, 43},
                    TestCaseWithoutAnswer{Bitboard{0x9c090a544c280040}, 42},
                    TestCaseWithoutAnswer{Bitboard{0x21010a151a405350}, 44},
                    TestCaseWithoutAnswer{Bitboard{0x4831b41488027311}, 3},
                    TestCaseWithoutAnswer{Bitboard{0x1012012370c1800}, 0},
                    TestCaseWithoutAnswer{Bitboard{0x1226004048909}, 52},
                    TestCaseWithoutAnswer{Bitboard{0x1213089000011c26}, 62},
                    TestCaseWithoutAnswer{Bitboard{0x88e4808200200c0c}, 49},
                    TestCaseWithoutAnswer{Bitboard{0x40180e2241e00cc0}, 32},
                    TestCaseWithoutAnswer{Bitboard{0x8200c0002ca0c488}, 55},
                    TestCaseWithoutAnswer{Bitboard{0x6600e00418318140}, 43}));
}  // namespace AttackMapTests
