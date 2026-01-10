#pragma once

#include <cstdint>
#include <vector>

#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine {
/**
 * \brief Class that all possible moves for a given position.
 *
 * \author nook0110
 */
class MoveGenerator {
 public:
  enum class Type : std::uint8_t {
    kCaptures,
    kQuiets,
    kEvasions,
    kNonEvasions,
    kLegal
  };

  static constexpr size_t kMaxMovesPerPosition = 218;
  MoveGenerator() { moves_.reserve(kMaxMovesPerPosition); }
  ~MoveGenerator();

  template <Type type>
  [[nodiscard]] auto GenerateMoves(const Position& position) const
      -> std::conditional_t<type == Type::kLegal, MoveList<LegalTag>,
                            MoveList<PseudoLegalTag>>;

 private:
  using Moves = std::vector<Move>;
  template <Player Us, Type GenType>
  void GenerateAll(Moves& moves, const Position& position) const;

  template <Player Us, Type GenType>
  void GeneratePawnMoves(Moves& moves, const Position& position,
                         Bitboard target) const;

  template <Player Us, Piece Pt>
  void GeneratePieceMoves(Moves& moves, const Position& position,
                          Bitboard target) const;

  void GenerateCastling(Moves& moves, const Position& position,
                        Player us) const;

  mutable Moves moves_;
};

}  // namespace SimpleChessEngine
