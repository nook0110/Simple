/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "position.h"
#include "attacks.h"

namespace Stockfish {
using namespace Attacks;
inline void add_dirty_threat(DirtyThreats* const dts,
                             bool                putPiece,
                             Piece               pc,
                             Piece               threatened,
                             Square              s,
                             Square              threatenedSq) {
    dts->list.push_back({pc, threatened, s, threatenedSq, putPiece});
}

#ifdef USE_AVX512ICL
// Given a DirtyThreat template and bit offsets to insert the piece type and square, write the threats
// present at the given bitboard.
template<int SqShift, int PcShift>
void write_multiple_dirties(const Position& p,
                            Bitboard        mask,
                            DirtyThreat     dt_template,
                            DirtyThreats*   dts) {
    static_assert(sizeof(DirtyThreat) == 4);

    const __m512i board    = _mm512_loadu_si512(p.piece_array().data());
    const int     dt_count = popcount(mask);
    assert(dt_count <= 16);

    const __m512i template_v = _mm512_set1_epi32(dt_template.raw());
    auto*         write      = dts->list.make_space(dt_count);

    // Extract the list of squares and upconvert to 32 bits. There are never more than 16
    // incoming threats so this is sufficient.
    __m512i threat_squares = _mm512_maskz_compress_epi8(mask, AllSquares);
    threat_squares         = _mm512_cvtepi8_epi32(_mm512_castsi512_si128(threat_squares));

    __m512i threat_pieces =
      _mm512_maskz_permutexvar_epi8(0x1111111111111111ULL, threat_squares, board);

    // Shift the piece and square into place
    threat_squares = _mm512_slli_epi32(threat_squares, SqShift);
    threat_pieces  = _mm512_slli_epi32(threat_pieces, PcShift);

    const __m512i dirties =
      _mm512_ternarylogic_epi32(template_v, threat_squares, threat_pieces, 254 /* A | B | C */);
    _mm512_storeu_si512(write, dirties);
}
#endif

constexpr bool can_slider_threat(Piece pc, Piece slider) {
    return type_of(pc) != QUEEN || type_of(slider) == QUEEN;
}

template<bool ComputeRay>
void Position::update_piece_threats(Piece               pc,
                                    bool                putPiece,
                                    Square              s,
                                    DirtyThreats* const dts,
                                    // Silence spurious warning on GCC 10
                                    [[maybe_unused]] Bitboard noRaysContaining) const {
    const Bitboard occupied         = pieces();
    const auto [bAttacks, rAttacks] = both_attacks_bb(s, occupied);
    const Bitboard  sliderAttacks   = bAttacks | rAttacks;
    const Bitboard  occupiedNoK     = occupied ^ pieces(KING);
    const PieceType pt              = type_of(pc);
    const Bitboard  sliders = (pieces(BISHOP, QUEEN) & bAttacks) | (pieces(ROOK, QUEEN) & rAttacks);

    auto process_sliders = [&](bool addDirectAttacks) {
        Bitboard b = sliders;
        while (b)
        {
            Square sliderSq = pop_lsb(b);
            Piece  slider   = piece_on(sliderSq);

            const Bitboard ray        = ray_pass_bb(sliderSq, s);
            const Bitboard discovered = ray & sliderAttacks & occupiedNoK;

            assert(!more_than_one(discovered));
            if (discovered && (ray & noRaysContaining) != noRaysContaining)
            {
                const Square threatenedSq = lsb(discovered);
                const Piece  threatenedPc = piece_on(threatenedSq);
                if (can_slider_threat(threatenedPc, slider))
                    add_dirty_threat(dts, !putPiece, slider, threatenedPc, sliderSq, threatenedSq);
            }

            if (addDirectAttacks && can_slider_threat(pc, slider))
                add_dirty_threat(dts, putPiece, slider, pc, sliderSq, s);
        }
    };

    // Kings emit no direct threats
    if (pt == KING)
    {
        if constexpr (ComputeRay)
            process_sliders(false);
        return;
    }

    const Bitboard threatTargets = pt == PAWN                 ? pieces(KNIGHT, ROOK)
                                 : pt == BISHOP || pt == ROOK ? pieces(PAWN, KNIGHT, BISHOP, ROOK)
                                                              : occupiedNoK;
    Bitboard       threatened    = (pt == BISHOP  ? bAttacks
                                    : pt == ROOK  ? rAttacks
                                    : pt == QUEEN ? sliderAttacks
                                    : pt == PAWN  ? PseudoAttacks[color_of(pc)][s]
                                                  : PseudoAttacks[pt][s])
                        & threatTargets;
    Bitboard incomingThreats = PseudoAttacks[KNIGHT][s] & pieces(KNIGHT);

    if (pt == KNIGHT || pt == ROOK)
        incomingThreats |= (attacks_bb<PAWN>(s, WHITE) & pieces(BLACK, PAWN))
                         | (attacks_bb<PAWN>(s, BLACK) & pieces(WHITE, PAWN));

#ifdef USE_AVX512ICL
    write_multiple_dirties<DirtyThreat::ThreatenedSqOffset, DirtyThreat::ThreatenedPcOffset>(
      *this, threatened, {pc, NO_PIECE, s, Square(0), putPiece}, dts);

    const Bitboard directSliders = pt == QUEEN ? sliders & pieces(QUEEN) : sliders;
    write_multiple_dirties<DirtyThreat::PcSqOffset, DirtyThreat::PcOffset>(
      *this, directSliders | incomingThreats, {NO_PIECE, pc, Square(0), s, putPiece}, dts);

    // For ICL, direct threats were written above
    if constexpr (ComputeRay)
        process_sliders(false);
#else
    while (threatened)
    {
        Square threatenedSq = pop_lsb(threatened);
        Piece  threatenedPc = piece_on(threatenedSq);
        assert(threatenedSq != s);
        assert(threatenedPc != NO_PIECE);

        add_dirty_threat(dts, putPiece, pc, threatenedPc, s, threatenedSq);
    }

    if constexpr (ComputeRay)
        process_sliders(true);
    else
        incomingThreats |= pt == QUEEN ? sliders & pieces(QUEEN) : sliders;

    while (incomingThreats)
    {
        Square srcSq = pop_lsb(incomingThreats);
        Piece  srcPc = piece_on(srcSq);
        assert(srcSq != s);
        assert(srcPc != NO_PIECE);

        add_dirty_threat(dts, putPiece, srcPc, pc, srcSq, s);
    }
#endif
}

template void Position::update_piece_threats<true>(Piece, bool, Square, DirtyThreats*, Bitboard) const;
template void Position::update_piece_threats<false>(Piece, bool, Square, DirtyThreats*, Bitboard) const;

}
