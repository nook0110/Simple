#include "Position.h"

namespace SimpleChessEngine {
[[nodiscard]] Eval TaperedEval::operator()(PhaseValue pv) const {
  const auto mg_limit =
      kPhaseValueLimits[static_cast<size_t>(GamePhase::kMiddleGame)];
  const auto eg_limit =
      kPhaseValueLimits[static_cast<size_t>(GamePhase::kEndGame)];
  pv = std::clamp(pv, eg_limit, mg_limit);
  return (eval[static_cast<size_t>(GamePhase::kMiddleGame)] * (pv - eg_limit) +
          eval[static_cast<size_t>(GamePhase::kEndGame)] * (mg_limit - pv)) /
         kLimitsDifference;
}

[[nodiscard]] Eval Position::Evaluate() const {
  const auto us = side_to_move_;
  const auto them = Flip(us);
  const auto us_idx = static_cast<size_t>(us);
  const auto them_idx = static_cast<size_t>(them);

  TaperedEval result{};
  result +=
      evaluation_data_.material[us_idx] - evaluation_data_.material[them_idx];
  result += evaluation_data_.psqt[us_idx] - evaluation_data_.psqt[them_idx];

  const Eval tapered = result(evaluation_data_.non_pawn_material);

  return tapered + kTempoBonus;
}
}  // namespace SimpleChessEngine