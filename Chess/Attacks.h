#pragma once

#include <memory>

#include "BitBoard.h"
#include "Piece.h"
#include "Utility.h"

namespace SimpleChessEngine
{
/**
 * \brief
 *
 */
struct Magic
{
  Bitboard mask;
  Bitboard magic;
  unsigned shift;
  [[nodiscard]] size_t GetIndex(const Bitboard occupancy) const
  {
    return static_cast<size_t>((mask & occupancy) * magic >> shift);
  }
};

constexpr size_t GetTableSize(const Piece sliding_piece)
{
  if (sliding_piece == Piece::kBishop) return 0x1480;
  if (sliding_piece == Piece::kRook) return 0x19000;
  return 0;
}

template <Piece sliding_piece, size_t table_size = GetTableSize(sliding_piece)>
class AttackTable
{
 public:
  static Bitboard GetAttackMap(BitIndex square, Bitboard occupied);

  AttackTable();

 private:
  static size_t GetAttackTableAddress(BitIndex square,
                                      Bitboard occupied = kEmptyBoard);

  std::array<Bitboard, table_size> table_ = {};
  std::array<size_t, kBoardArea> base_ = {};
  std::array<Magic, kBoardArea> magic_ = {};
  static inline const std::unique_ptr<AttackTable> self_ =
      std::make_unique<AttackTable>();
};

template <Piece piece, size_t table_size>
size_t AttackTable<piece, table_size>::GetAttackTableAddress(
    const BitIndex square, Bitboard occupied)
{
  assert(IsWeakSlidingPiece(piece));
  return self_->base_[square] + self_->magic_[square].GetIndex(occupied);
}

template <Piece piece, size_t table_size>
Bitboard AttackTable<piece, table_size>::GetAttackMap(const BitIndex square,
                                                      const Bitboard occupied)
{
  static constexpr std::array king_attacks = {770ull,
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
  static constexpr std::array knight_attacks = {132096ull,
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
  if constexpr (piece == Piece::kKnight)
  {
    return Bitboard{knight_attacks[static_cast<size_t>(square)]};
  }
  if constexpr (piece == Piece::kKing)
  {
    return Bitboard{king_attacks[static_cast<size_t>(square)]};
  }
  if constexpr (piece == Piece::kQueen)
  {
    return AttackTable<Piece::kBishop>::GetAttackMap(square, occupied) |
           AttackTable<Piece::kRook>::GetAttackMap(square, occupied);
  }
  if constexpr (IsWeakSlidingPiece(piece))
  {
    return self_->table_[GetAttackTableAddress(square, occupied)];
  }
  assert(false);
  return {};
}

template class AttackTable<Piece::kKnight>;
template class AttackTable<Piece::kBishop>;
template class AttackTable<Piece::kRook>;
template class AttackTable<Piece::kQueen>;
template class AttackTable<Piece::kKing>;

template <Piece sliding_piece>
void InitBetween();
}  // namespace SimpleChessEngine