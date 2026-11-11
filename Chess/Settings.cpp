#include "Settings.h"

namespace SimpleChessEngine {
static std::ostream& operator<<(
    std::ostream& out, Settings::PruneParameters::RFPSettings settings) {
  out << "\tRFP: " << std::boolalpha << settings.kEnabled << "\n";
  if constexpr (settings.kEnabled) {
    out << "\t\tkDepthLimit: " << +settings.kDepthLimit << "\n";
    out << "\t\tkThreshold: " << settings.kThreshold << "\n";
  }
  return out;
}
static std::ostream& operator<<(
    std::ostream& out, Settings::PruneParameters::NMPSettings settings) {
  out << "\tNMP: " << std::boolalpha << settings.kEnabled << "\n";
  if constexpr (settings.kEnabled) {
    out << "\t\tkNullMoveReduction: " << +settings.kNullMoveReduction << "\n";
  }
  return out;
}
std::ostream& operator<<(std::ostream& out,
                                Settings::PruneParameters parameters) {
  out << "PruneParameters:\n";
  out << parameters.kRFPSettings << parameters.kNMPSettings;
  return out;
}
static std::ostream& operator<<(
    std::ostream& out,
    Settings::SearchParameters::AspirationWindowSettings settings) {
  out << "\tAspirationWindowSettings:\n";
  out << "\t\tkInitialDelta: " << settings.kInitialDelta << "\n";
  out << "\t\tkThreadDelta: " << settings.kThreadDelta << "\n";
  out << "\t\tkDeltaGrowth: " << settings.kDeltaGrowth << "\n";
  out << "\t\tkMeanSquaredScoreDivisor: "
      << settings.kMeanSquaredScoreDivisor << "\n";
  return out;
}

std::ostream& operator<<(std::ostream& out,
                                Settings::SearchParameters parameters) {
  out << "SearchParameters:\n";
  out << parameters.kAspirationWindowSettings;

  return out;
}
}  // namespace SimpleChessEngine
