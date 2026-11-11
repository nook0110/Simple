#include <gtest/gtest.h>

#include <random>

#include "Attacks.h"
#include "Hasher.h"
#include "PSQT.h"

using namespace SimpleChessEngine;

namespace {
constexpr std::array kLegacyKing = {770ull,
                                    1797ull,
                                    3594ull,
                                    7188ull,
                                    14376ull,
                                    28752ull,
                                    57504ull,
                                    49216ull,
                                    197123ull,
                                    460039ull,
                                    920078ull,
                                    1840156ull,
                                    3680312ull,
                                    7360624ull,
                                    14721248ull,
                                    12599488ull,
                                    50463488ull,
                                    117769984ull,
                                    235539968ull,
                                    471079936ull,
                                    942159872ull,
                                    1884319744ull,
                                    3768639488ull,
                                    3225468928ull,
                                    12918652928ull,
                                    30149115904ull,
                                    60298231808ull,
                                    120596463616ull,
                                    241192927232ull,
                                    482385854464ull,
                                    964771708928ull,
                                    825720045568ull,
                                    3307175149568ull,
                                    7718173671424ull,
                                    15436347342848ull,
                                    30872694685696ull,
                                    61745389371392ull,
                                    123490778742784ull,
                                    246981557485568ull,
                                    211384331665408ull,
                                    846636838289408ull,
                                    1975852459884544ull,
                                    3951704919769088ull,
                                    7903409839538176ull,
                                    15806819679076352ull,
                                    31613639358152704ull,
                                    63227278716305408ull,
                                    54114388906344448ull,
                                    216739030602088448ull,
                                    505818229730443264ull,
                                    1011636459460886528ull,
                                    2023272918921773056ull,
                                    4046545837843546112ull,
                                    8093091675687092224ull,
                                    16186183351374184448ull,
                                    13853283560024178688ull,
                                    144959613005987840ull,
                                    362258295026614272ull,
                                    724516590053228544ull,
                                    1449033180106457088ull,
                                    2898066360212914176ull,
                                    5796132720425828352ull,
                                    11592265440851656704ull,
                                    4665729213955833856ull};
constexpr std::array kLegacyKnight = {132096ull,
                                      329728ull,
                                      659712ull,
                                      1319424ull,
                                      2638848ull,
                                      5277696ull,
                                      10489856ull,
                                      4202496ull,
                                      33816580ull,
                                      84410376ull,
                                      168886289ull,
                                      337772578ull,
                                      675545156ull,
                                      1351090312ull,
                                      2685403152ull,
                                      1075839008ull,
                                      8657044482ull,
                                      21609056261ull,
                                      43234889994ull,
                                      86469779988ull,
                                      172939559976ull,
                                      345879119952ull,
                                      687463207072ull,
                                      275414786112ull,
                                      2216203387392ull,
                                      5531918402816ull,
                                      11068131838464ull,
                                      22136263676928ull,
                                      44272527353856ull,
                                      88545054707712ull,
                                      175990581010432ull,
                                      70506185244672ull,
                                      567348067172352ull,
                                      1416171111120896ull,
                                      2833441750646784ull,
                                      5666883501293568ull,
                                      11333767002587136ull,
                                      22667534005174272ull,
                                      45053588738670592ull,
                                      18049583422636032ull,
                                      145241105196122112ull,
                                      362539804446949376ull,
                                      725361088165576704ull,
                                      1450722176331153408ull,
                                      2901444352662306816ull,
                                      5802888705324613632ull,
                                      11533718717099671552ull,
                                      4620693356194824192ull,
                                      288234782788157440ull,
                                      576469569871282176ull,
                                      1224997833292120064ull,
                                      2449995666584240128ull,
                                      4899991333168480256ull,
                                      9799982666336960512ull,
                                      1152939783987658752ull,
                                      2305878468463689728ull,
                                      1128098930098176ull,
                                      2257297371824128ull,
                                      4796069720358912ull,
                                      9592139440717824ull,
                                      19184278881435648ull,
                                      38368557762871296ull,
                                      4679521487814656ull,
                                      9077567998918656ull};

constexpr std::array kDirections = {Compass::kNorthWest, Compass::kSouthWest,
                                    Compass::kSouthEast, Compass::kNorthEast,
                                    Compass::kNorth,     Compass::kWest,
                                    Compass::kSouth,     Compass::kEast};
}

TEST(ConstantTables, LeapersMatchOriginalLiterals) {
  for (BitIndex square = 0; square < 64; ++square) {
    EXPECT_EQ(AttackTable<Piece::kKing>::GetAttackMap(square, {}),
              Bitboard{kLegacyKing[square]});
    EXPECT_EQ(AttackTable<Piece::kKnight>::GetAttackMap(square, {}),
              Bitboard{kLegacyKnight[square]});
  }
}

TEST(ConstantTables, GeometryMatchesOriginalGenerators) {
  std::array<std::array<Bitboard, kBoardArea>, kBoardArea> between{};
  auto ray = between;
  auto rook_between_reference = between;
  for (BitIndex square = 0; square < 64; ++square) {
    for (const auto direction : kDirections) {
      const bool orthogonal =
          direction == Compass::kNorth || direction == Compass::kSouth ||
          direction == Compass::kWest || direction == Compass::kEast;
      Bitboard result{};
      BitIndex target = square;
      for (Bitboard step{~kEmptyBoard}; step.Any(); result |= step) {
        between[square][target] |= result;
        if (orthogonal) rook_between_reference[square][target] = result;
        step = DoShiftIfValid(target, direction).value_or(Bitboard{});
      }
      target = square;
      while (DoShiftIfValid(target, direction)) ray[square][target] |= result;
    }
    for (BitIndex target = 0; target < 64; ++target) {
      EXPECT_EQ(Between(square, target), between[square][target]);
      EXPECT_EQ(rook_between[square][target],
                rook_between_reference[square][target]);
      EXPECT_EQ(Ray(square, target), ray[square][target]);
    }
  }
}

TEST(ConstantTables, PawnsMatchOriginalGenerator) {
  for (auto color : {Player::kWhite, Player::kBlack}) {
    for (BitIndex square = 0; square < 64; ++square) {
      Bitboard expected{};
      for (const auto direction :
           kPawnAttackDirections[static_cast<size_t>(color)]) {
        expected |= GetShiftIfValid(square, direction).value_or(Bitboard{});
      }
      EXPECT_EQ(GetPawnAttacks(square, color), expected);
    }
  }
}

TEST(ConstantTables, PsqtMatchesOriginalMirror) {
  for (size_t piece = 0; piece < kPieceTypes; ++piece) {
    for (BitIndex square = 0; square < 64; ++square) {
      const auto [file, rank] = GetCoordinates(square);
      EXPECT_EQ(kPSQT[1][piece][GetSquareIndex(file, 7 - rank)],
                kPSQT[0][piece][square]);
    }
  }
}

template <Piece piece>
void CheckSliderSubsets() {
  for (BitIndex square = 0; square < 64; ++square) {
    Bitboard mask{};
    const size_t first = piece == Piece::kBishop ? 0 : 4;
    for (size_t i = first; i < first + 4; ++i) {
      auto target = square;
      while (const auto step = DoShiftIfValid(target, kDirections[i])) {
        if (GetShiftIfValid(target, kDirections[i])) mask |= *step;
      }
    }
    Bitboard subset{};
    do {
      ASSERT_EQ(AttackTable<piece>::GetAttackMap(square, subset),
                GenerateAttackMask<piece>(square, subset));
      ASSERT_EQ(AttackTable<piece>::GetAttackMap(square, subset | ~mask),
                GenerateAttackMask<piece>(square, subset));
      subset = (subset - mask) & mask;
    } while (subset.Any());
  }
}

TEST(ConstantTables, SliderSubsetsMatchOriginalGenerator) {
  CheckSliderSubsets<Piece::kBishop>();
  CheckSliderSubsets<Piece::kRook>();
}
