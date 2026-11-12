#include "Settings.h"

namespace SimpleChessEngine {
static std::ostream& operator<<(
    std::ostream& out, Settings::PruneParameters::RFPSettings settings) {
  out << "\tRFP: " << std::boolalpha << settings.kEnabled << "\n";
  if constexpr (settings.kEnabled) {
    out << "\t\tkDepthLimit: " << +settings.depth_limit << "\n";
    out << "\t\tkThreshold: " << settings.threshold << "\n";
  }
  return out;
}
static std::ostream& operator<<(
    std::ostream& out, Settings::PruneParameters::NMPSettings settings) {
  out << "\tNMP: " << std::boolalpha << settings.kEnabled << "\n";
  if constexpr (settings.kEnabled) {
    out << "\t\tkNullMoveReduction: " << +settings.null_move_reduction << "\n";
  }
  return out;
}
std::ostream& operator<<(std::ostream& out, Settings::PruneParameters) {
  out << "PruneParameters:\n";
  out << Settings::current<Settings::PruneParameters::RFPSettings>;
  out << Settings::current<Settings::PruneParameters::NMPSettings>;
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
