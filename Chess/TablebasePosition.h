#pragma once

#include <optional>

#include "Position.h"
#include "Tablebase.h"

namespace SimpleChessEngine::Tablebase {

class PositionBuilder {
 public:
  enum class BuildError : std::uint8_t {
    kNone,
    kInvalidMaterial,
    kInvalidSquare,
    kOverlappingPieces,
    kAdjacentKings,
    kInvalidPiece,
    kPawnOnBackRank,
    kSideNotToMoveInCheck,
  };

  [[nodiscard]] static std::optional<Position> Build(
      const CanonicalPosition& encoded);
  [[nodiscard]] static BuildError Diagnose(const CanonicalPosition& encoded);
  [[nodiscard]] static bool IsCanonicalLegalRaw(const MaterialClass& material,
                                                std::uint64_t raw_index);
  static void ClearEnPassant(Position& position);

 private:
  [[nodiscard]] static std::optional<Position> BuildImpl(
      const CanonicalPosition& encoded, BuildError* error);
};

}  // namespace SimpleChessEngine::Tablebase
