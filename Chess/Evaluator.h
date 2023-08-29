#pragma once

#include "Position.h"

namespace SimpleChessEngine
{
using Eval = int;

enum class GamePhase
{
  kMiddleGame,
  kEndGame
};

constexpr size_t kGamePhases = 2;

using PhaseValue = int;

constexpr std::array<PhaseValue, kGamePhases> kPhaseValueLimits = {};

struct TaperedEval
{
  std::array<Eval, kGamePhases> eval;
  [[nodiscard]] Eval operator()(PhaseValue pv) const
  {
    /*
    if (pv < kPhaseValueLimits[???0]) pv = kPhaseValueLimits[???0];
    if (pv > kPhaseValueLimits[???1]) pv = kPhaseValueLimits[???1];
    return eval[???0] * (kPhaseValueLimits[???1] - pv) +
           eval[???1] * (pv - kPhaseValueLimits[???0]);
           */
  }
};

class Evaluator
{
 public:
  explicit Evaluator() {}

  Eval operator()(const Position& position, Eval alpha, Eval beta) const;

  [[nodiscard]] static Eval GetGameResult(const Position& position)
  {
    // assert(MoveGenerator{}(const_cast<Position&>(position)).empty());
    return -10000;
  }
};

inline Eval Evaluator::operator()(const Position& position, Eval alpha,
                                  Eval beta) const
{
  auto eval = Eval{};

  constexpr std::array piece_value{0, 100, 250, 350, 550, 1000, 0};

  // get all pieces
  auto pieces = position.GetPieces(Player::kWhite);

  // generate moves for each piece
  while (const auto from = pieces.GetFirstBit())
  {
    // get first piece
    pieces.Reset(*from);

    eval += piece_value[static_cast<size_t>(position.GetPiece(*from))];
  }

  // get all pieces
  pieces = position.GetPieces(Player::kBlack);

  // generate moves for each piece
  while (const auto from = pieces.GetFirstBit())
  {
    // get first piece
    pieces.Reset(*from);

    eval -= piece_value[static_cast<size_t>(position.GetPiece(*from))];
  }

  eval += position.GetSideToMove() == Player::kWhite ? 10 : -10;

  return position.GetSideToMove() == Player::kWhite ? eval : -eval;
}  // namespace SimpleChessEngine
}  // namespace SimpleChessEngine