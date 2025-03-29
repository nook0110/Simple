#include "Attacks.h"

using namespace SimpleChessEngine;

template <Piece sliding_piece>
constexpr std::array<Compass, 4> GetStepDelta() {
  if constexpr (sliding_piece == Piece::kBishop) {
    return {Compass::kNorthWest, Compass::kSouthWest, Compass::kSouthEast,
            Compass::kNorthEast};
  }
  if constexpr (sliding_piece == Piece::kRook) {
    return {Compass::kNorth, Compass::kWest, Compass::kSouth, Compass::kEast};
  }

  assert(false);
  return {};
}

template <Piece sliding_piece>
Bitboard SimpleChessEngine::GenerateAttackMask(const BitIndex square,
                                               const Bitboard occupancy) {
  assert(IsWeakSlidingPiece(sliding_piece));
  assert(IsOk(square));

  Bitboard result = kEmptyBoard;

  for (auto direction : GetStepDelta<sliding_piece>()) {
    Bitboard step;
    for (BitIndex temp = square;
         (occupancy & GetBitboardOfSquare(temp)).None();) {
      step = DoShiftIfValid(temp, direction).value_or(kEmptyBoard);
      result |= step;
      if (step.None()) break;
    }
  }
  return result;
}

template <Piece sliding_piece>
void SimpleChessEngine::InitBetween() {
  assert(IsWeakSlidingPiece(sliding_piece));
  for (BitIndex sq = 0; sq < kBoardArea; ++sq) {
    for (auto direction : GetStepDelta<sliding_piece>()) {
      Bitboard result{};
      BitIndex temp = sq;
      for (Bitboard step{~kEmptyBoard}; step.Any(); result |= step) {
        if constexpr (sliding_piece == Piece::kBishop)
          bishop_between[sq][temp] = result;
        else
          rook_between[sq][temp] = result;
        step = DoShiftIfValid(temp, direction).value_or(Bitboard{});
      }
      temp = sq;
      while (DoShiftIfValid(temp, direction)) {
        if constexpr (sliding_piece == Piece::kBishop)
          bishop_ray[sq][temp] = result;
        else
          rook_ray[sq][temp] = result;
      }
    }
  }
}

template void SimpleChessEngine::InitBetween<Piece::kBishop>();
template void SimpleChessEngine::InitBetween<Piece::kRook>();

template <Piece sliding_piece>
const std::unique_ptr<AttackTable<sliding_piece>>
    AttackTable<sliding_piece>::self_ = std::make_unique<AttackTable>();

namespace {
std::array<Bitboard, 88772> shared_table;
}

template <Piece sliding_piece>
AttackTable<sliding_piece>::AttackTable() {
  if constexpr (!IsWeakSlidingPiece(sliding_piece)) return;

  table_ = shared_table.data();

  if constexpr (sliding_piece == Piece::kBishop) {
    magic_ = {{{Bitboard{0x0040201008040200}, Bitboard{0x007fbfbfbfbfbfff},
                size_t{5378}},
               {Bitboard{0x0000402010080400}, Bitboard{0x0000a060401007fc},
                size_t{4093}},
               {Bitboard{0x0000004020100a00}, Bitboard{0x0001004008020000},
                size_t{4314}},
               {Bitboard{0x0000000040221400}, Bitboard{0x0000806004000000},
                size_t{6587}},
               {Bitboard{0x0000000002442800}, Bitboard{0x0000100400000000},
                size_t{6491}},
               {Bitboard{0x0000000204085000}, Bitboard{0x000021c100b20000},
                size_t{6330}},
               {Bitboard{0x0000020408102000}, Bitboard{0x0000040041008000},
                size_t{5609}},
               {Bitboard{0x0002040810204000}, Bitboard{0x00000fb0203fff80},
                size_t{22236}},
               {Bitboard{0x0020100804020000}, Bitboard{0x0000040100401004},
                size_t{6106}},
               {Bitboard{0x0040201008040000}, Bitboard{0x0000020080200802},
                size_t{5625}},
               {Bitboard{0x00004020100a0000}, Bitboard{0x0000004010202000},
                size_t{16785}},
               {Bitboard{0x0000004022140000}, Bitboard{0x0000008060040000},
                size_t{16817}},
               {Bitboard{0x0000000244280000}, Bitboard{0x0000004402000000},
                size_t{6842}},
               {Bitboard{0x0000020408500000}, Bitboard{0x0000000801008000},
                size_t{7003}},
               {Bitboard{0x0002040810200000}, Bitboard{0x000007efe0bfff80},
                size_t{4197}},
               {Bitboard{0x0004081020400000}, Bitboard{0x0000000820820020},
                size_t{7356}},
               {Bitboard{0x0010080402000200}, Bitboard{0x0000400080808080},
                size_t{4602}},
               {Bitboard{0x0020100804000400}, Bitboard{0x00021f0100400808},
                size_t{4538}},
               {Bitboard{0x004020100a000a00}, Bitboard{0x00018000c06f3fff},
                size_t{29531}},
               {Bitboard{0x0000402214001400}, Bitboard{0x0000258200801000},
                size_t{45393}},
               {Bitboard{0x0000024428002800}, Bitboard{0x0000240080840000},
                size_t{12420}},
               {Bitboard{0x0002040850005000}, Bitboard{0x000018000c03fff8},
                size_t{15763}},
               {Bitboard{0x0004081020002000}, Bitboard{0x00000a5840208020},
                size_t{5050}},
               {Bitboard{0x0008102040004000}, Bitboard{0x0000020008208020},
                size_t{4346}},
               {Bitboard{0x0008040200020400}, Bitboard{0x0000804000810100},
                size_t{6074}},
               {Bitboard{0x0010080400040800}, Bitboard{0x0001011900802008},
                size_t{7866}},
               {Bitboard{0x0020100a000a1000}, Bitboard{0x0000804000810100},
                size_t{32139}},
               {Bitboard{0x0040221400142200}, Bitboard{0x000100403c0403ff},
                size_t{57673}},
               {Bitboard{0x0002442800284400}, Bitboard{0x00078402a8802000},
                size_t{55365}},
               {Bitboard{0x0004085000500800}, Bitboard{0x0000101000804400},
                size_t{15818}},
               {Bitboard{0x0008102000201000}, Bitboard{0x0000080800104100},
                size_t{5562}},
               {Bitboard{0x0010204000402000}, Bitboard{0x00004004c0082008},
                size_t{6390}},
               {Bitboard{0x0004020002040800}, Bitboard{0x0001010120008020},
                size_t{7930}},
               {Bitboard{0x0008040004081000}, Bitboard{0x000080809a004010},
                size_t{13329}},
               {Bitboard{0x00100a000a102000}, Bitboard{0x0007fefe08810010},
                size_t{7170}},
               {Bitboard{0x0022140014224000}, Bitboard{0x0003ff0f833fc080},
                size_t{27267}},
               {Bitboard{0x0044280028440200}, Bitboard{0x007fe08019003042},
                size_t{53787}},
               {Bitboard{0x0008500050080400}, Bitboard{0x003fffefea003000},
                size_t{5097}},
               {Bitboard{0x0010200020100800}, Bitboard{0x0000101010002080},
                size_t{6643}},
               {Bitboard{0x0020400040201000}, Bitboard{0x0000802005080804},
                size_t{6138}},
               {Bitboard{0x0002000204081000}, Bitboard{0x0000808080a80040},
                size_t{7418}},
               {Bitboard{0x0004000408102000}, Bitboard{0x0000104100200040},
                size_t{7898}},
               {Bitboard{0x000a000a10204000}, Bitboard{0x0003ffdf7f833fc0},
                size_t{42012}},
               {Bitboard{0x0014001422400000}, Bitboard{0x0000008840450020},
                size_t{57350}},
               {Bitboard{0x0028002844020000}, Bitboard{0x00007ffc80180030},
                size_t{22813}},
               {Bitboard{0x0050005008040200}, Bitboard{0x007fffdd80140028},
                size_t{56693}},
               {Bitboard{0x0020002010080400}, Bitboard{0x00020080200a0004},
                size_t{5818}},
               {Bitboard{0x0040004020100800}, Bitboard{0x0000101010100020},
                size_t{7098}},
               {Bitboard{0x0000020408102000}, Bitboard{0x0007ffdfc1805000},
                size_t{4451}},
               {Bitboard{0x0000040810204000}, Bitboard{0x0003ffefe0c02200},
                size_t{4709}},
               {Bitboard{0x00000a1020400000}, Bitboard{0x0000000820806000},
                size_t{4794}},
               {Bitboard{0x0000142240000000}, Bitboard{0x0000000008403000},
                size_t{13364}},
               {Bitboard{0x0000284402000000}, Bitboard{0x0000000100202000},
                size_t{4570}},
               {Bitboard{0x0000500804020000}, Bitboard{0x0000004040802000},
                size_t{4282}},
               {Bitboard{0x0000201008040200}, Bitboard{0x0004010040100400},
                size_t{14964}},
               {Bitboard{0x0000402010080400}, Bitboard{0x00006020601803f4},
                size_t{4026}},
               {Bitboard{0x0002040810204000}, Bitboard{0x0003ffdfdfc28048},
                size_t{4826}},
               {Bitboard{0x0004081020400000}, Bitboard{0x0000000820820020},
                size_t{7354}},
               {Bitboard{0x000a102040000000}, Bitboard{0x0000000008208060},
                size_t{4848}},
               {Bitboard{0x0014224000000000}, Bitboard{0x0000000000808020},
                size_t{15946}},
               {Bitboard{0x0028440200000000}, Bitboard{0x0000000001002020},
                size_t{14932}},
               {Bitboard{0x0050080402000000}, Bitboard{0x0000000401002008},
                size_t{16588}},
               {Bitboard{0x0020100804020000}, Bitboard{0x0000004040404040},
                size_t{6905}},
               {Bitboard{0x0040201008040200}, Bitboard{0x007fff9fdf7ff813},
                size_t{16076}}}};
  }

  if constexpr (sliding_piece == Piece::kRook) {
    magic_ = {{{Bitboard{0x000101010101017e}, Bitboard{0x00280077ffebfffe},
                size_t{26304}},
               {Bitboard{0x000202020202027c}, Bitboard{0x2004010201097fff},
                size_t{35520}},
               {Bitboard{0x000404040404047a}, Bitboard{0x0010020010053fff},
                size_t{38592}},
               {Bitboard{0x0008080808080876}, Bitboard{0x0040040008004002},
                size_t{8026}},
               {Bitboard{0x001010101010106e}, Bitboard{0x7fd00441ffffd003},
                size_t{22196}},
               {Bitboard{0x002020202020205e}, Bitboard{0x4020008887dffffe},
                size_t{80870}},
               {Bitboard{0x004040404040403e}, Bitboard{0x004000888847ffff},
                size_t{76747}},
               {Bitboard{0x008080808080807e}, Bitboard{0x006800fbff75fffd},
                size_t{30400}},
               {Bitboard{0x0001010101017e00}, Bitboard{0x000028010113ffff},
                size_t{11115}},
               {Bitboard{0x0002020202027c00}, Bitboard{0x0020040201fcffff},
                size_t{18205}},
               {Bitboard{0x0004040404047a00}, Bitboard{0x007fe80042ffffe8},
                size_t{53577}},
               {Bitboard{0x0008080808087600}, Bitboard{0x00001800217fffe8},
                size_t{62724}},
               {Bitboard{0x0010101010106e00}, Bitboard{0x00001800073fffe8},
                size_t{34282}},
               {Bitboard{0x0020202020205e00}, Bitboard{0x00001800e05fffe8},
                size_t{29196}},
               {Bitboard{0x0040404040403e00}, Bitboard{0x00001800602fffe8},
                size_t{23806}},
               {Bitboard{0x0080808080807e00}, Bitboard{0x000030002fffffa0},
                size_t{49481}},
               {Bitboard{0x00010101017e0100}, Bitboard{0x00300018010bffff},
                size_t{2410}},
               {Bitboard{0x00020202027c0200}, Bitboard{0x0003000c0085fffb},
                size_t{36498}},
               {Bitboard{0x00040404047a0400}, Bitboard{0x0004000802010008},
                size_t{24478}},
               {Bitboard{0x0008080808760800}, Bitboard{0x0004002020020004},
                size_t{10074}},
               {Bitboard{0x00101010106e1000}, Bitboard{0x0001002002002001},
                size_t{79315}},
               {Bitboard{0x00202020205e2000}, Bitboard{0x0001001000801040},
                size_t{51779}},
               {Bitboard{0x00404040403e4000}, Bitboard{0x0000004040008001},
                size_t{13586}},
               {Bitboard{0x00808080807e8000}, Bitboard{0x0000006800cdfff4},
                size_t{19323}},
               {Bitboard{0x000101017e010100}, Bitboard{0x0040200010080010},
                size_t{70612}},
               {Bitboard{0x000202027c020200}, Bitboard{0x0000080010040010},
                size_t{83652}},
               {Bitboard{0x000404047a040400}, Bitboard{0x0004010008020008},
                size_t{63110}},
               {Bitboard{0x0008080876080800}, Bitboard{0x0000040020200200},
                size_t{34496}},
               {Bitboard{0x001010106e101000}, Bitboard{0x0002008010100100},
                size_t{84966}},
               {Bitboard{0x002020205e202000}, Bitboard{0x0000008020010020},
                size_t{54341}},
               {Bitboard{0x004040403e404000}, Bitboard{0x0000008020200040},
                size_t{60421}},
               {Bitboard{0x008080807e808000}, Bitboard{0x0000820020004020},
                size_t{86402}},
               {Bitboard{0x0001017e01010100}, Bitboard{0x00fffd1800300030},
                size_t{50245}},
               {Bitboard{0x0002027c02020200}, Bitboard{0x007fff7fbfd40020},
                size_t{76622}},
               {Bitboard{0x0004047a04040400}, Bitboard{0x003fffbd00180018},
                size_t{84676}},
               {Bitboard{0x0008087608080800}, Bitboard{0x001fffde80180018},
                size_t{78757}},
               {Bitboard{0x0010106e10101000}, Bitboard{0x000fffe0bfe80018},
                size_t{37346}},
               {Bitboard{0x0020205e20202000}, Bitboard{0x0001000080202001},
                size_t{370}},
               {Bitboard{0x0040403e40404000}, Bitboard{0x0003fffbff980180},
                size_t{42182}},
               {Bitboard{0x0080807e80808000}, Bitboard{0x0001fffdff9000e0},
                size_t{45385}},
               {Bitboard{0x00017e0101010100}, Bitboard{0x00fffefeebffd800},
                size_t{61659}},
               {Bitboard{0x00027c0202020200}, Bitboard{0x007ffff7ffc01400},
                size_t{12790}},
               {Bitboard{0x00047a0404040400}, Bitboard{0x003fffbfe4ffe800},
                size_t{16762}},
               {Bitboard{0x0008760808080800}, Bitboard{0x001ffff01fc03000},
                size_t{0}},
               {Bitboard{0x00106e1010101000}, Bitboard{0x000fffe7f8bfe800},
                size_t{38380}},
               {Bitboard{0x00205e2020202000}, Bitboard{0x0007ffdfdf3ff808},
                size_t{11098}},
               {Bitboard{0x00403e4040404000}, Bitboard{0x0003fff85fffa804},
                size_t{21803}},
               {Bitboard{0x00807e8080808000}, Bitboard{0x0001fffd75ffa802},
                size_t{39189}},
               {Bitboard{0x007e010101010100}, Bitboard{0x00ffffd7ffebffd8},
                size_t{58628}},
               {Bitboard{0x007c020202020200}, Bitboard{0x007fff75ff7fbfd8},
                size_t{44116}},
               {Bitboard{0x007a040404040400}, Bitboard{0x003fff863fbf7fd8},
                size_t{78357}},
               {Bitboard{0x0076080808080800}, Bitboard{0x001fffbfdfd7ffd8},
                size_t{44481}},
               {Bitboard{0x006e101010101000}, Bitboard{0x000ffff810280028},
                size_t{64134}},
               {Bitboard{0x005e202020202000}, Bitboard{0x0007ffd7f7feffd8},
                size_t{41759}},
               {Bitboard{0x003e404040404000}, Bitboard{0x0003fffc0c480048},
                size_t{1394}},
               {Bitboard{0x007e808080808000}, Bitboard{0x0001ffffafd7ffd8},
                size_t{40910}},
               {Bitboard{0x7e01010101010100}, Bitboard{0x00ffffe4ffdfa3ba},
                size_t{66516}},
               {Bitboard{0x7c02020202020200}, Bitboard{0x007fffef7ff3d3da},
                size_t{3897}},
               {Bitboard{0x7a04040404040400}, Bitboard{0x003fffbfdfeff7fa},
                size_t{3930}},
               {Bitboard{0x7608080808080800}, Bitboard{0x001fffeff7fbfc22},
                size_t{72934}},
               {Bitboard{0x6e10101010101000}, Bitboard{0x0000020408001001},
                size_t{72662}},
               {Bitboard{0x5e20202020202000}, Bitboard{0x0007fffeffff77fd},
                size_t{56325}},
               {Bitboard{0x3e40404040404000}, Bitboard{0x0003ffffbf7dfeec},
                size_t{66501}},
               {Bitboard{0x7e80808080808000}, Bitboard{0x0001ffff9dffa333},
                size_t{14826}}}};
  }

  for (BitIndex square = 0; square < kBoardArea; ++square) {
    Bitboard mask_subset(kEmptyBoard);
    do {
      table_[magic_[square].GetAddress<sliding_piece>(mask_subset)] =
          GenerateAttackMask<sliding_piece>(square, mask_subset);
      mask_subset -= magic_[square].mask;
      mask_subset &= magic_[square].mask;
    } while (mask_subset.Any());
  }
}