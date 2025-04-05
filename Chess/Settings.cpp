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
  out << "\tRFP: " << std::boolalpha << settings.kEnabled << "\n";
  if constexpr (settings.kEnabled) {
    out << "\t\tkNullMoveReduction: " << +settings.kNullMoveReduction << "\n";
  }
  return out;
}
static std::ostream& operator<<(std::ostream& out,
                                Settings::PruneParameters parameters) {
  out << "PruneParameters:\n";
  out << parameters.kRFPSettings << parameters.kNMPSettings;
  return out;
}
static std::ostream& operator<<(
    std::ostream& out,
    Settings::SearchParameters::AspirationWindowSettings::Strategy strategy) {
  return out << (strategy == Settings::SearchParameters::
                                 AspirationWindowSettings::Strategy::Linear
                     ? "Linear"
                     : "Exponential");
}

static std::ostream& operator<<(
    std::ostream& out,
    Settings::SearchParameters::AspirationWindowSettings settings) {
  out << "\tAspirationWindowSettings:";
  out << "\t\tkEnabled: " << std::boolalpha << settings.kEnabled << "\n";
  if constexpr (settings.kEnabled) {
    out << "\t\tkDelta: " << settings.kDelta << "\n";
    out << "\t\tkStrategy: " << settings.kStrategy << "\n";
  }
  return out;
}

inline std::ostream& operator<<(std::ostream& out,
                                Settings::SearchParameters parameters) {
  out << "SearchParameters:\n";
  out << parameters.kAspirationWindowSettings;

  return out;
}
}  // namespace SimpleChessEngine
