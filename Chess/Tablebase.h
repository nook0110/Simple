#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <optional>
#include <vector>

#include "Piece.h"
#include "Player.h"
#include "Utility.h"

namespace SimpleChessEngine {
class Position;

namespace Tablebase {

enum class Wdl : std::int8_t {
  kLoss = -2,
  kBlessedLoss = -1,
  kDraw = 0,
  kCursedWin = 1,
  kWin = 2,
};

enum class PieceCode : std::uint8_t {
  kNone,
  kWhitePawn,
  kWhiteKnight,
  kWhiteBishop,
  kWhiteRook,
  kWhiteQueen,
  kBlackPawn,
  kBlackKnight,
  kBlackBishop,
  kBlackRook,
  kBlackQueen,
};

struct MaterialClass {
  std::array<PieceCode, 2> extras{PieceCode::kNone, PieceCode::kNone};
  std::uint8_t extra_count{};

  auto operator<=>(const MaterialClass&) const = default;

  [[nodiscard]] bool HasPawns() const;
  [[nodiscard]] std::uint8_t PieceCount() const { return extra_count + 2; }
};

struct CanonicalPosition {
  MaterialClass material;
  std::array<BitIndex, 4> squares{};
  Player side_to_move{Player::kWhite};

  bool operator==(const CanonicalPosition&) const = default;
};

struct ProbeResult {
  Wdl wdl{Wdl::kDraw};
  std::int16_t dtz{};

  bool operator==(const ProbeResult&) const = default;
};

struct CanonicalizationTrace {
  MaterialClass original_material;
  MaterialClass swapped_material;
  CanonicalPosition original_geometry;
  CanonicalPosition swapped_geometry;
};

[[nodiscard]] PieceCode EncodePiece(Piece piece, Player color);
[[nodiscard]] Piece DecodePiece(PieceCode code);
[[nodiscard]] Player DecodeColor(PieceCode code);
[[nodiscard]] PieceCode FlipColor(PieceCode code);
[[nodiscard]] std::uint16_t MaterialKey(const MaterialClass& material);
[[nodiscard]] std::vector<MaterialClass> AllMaterialClasses();

class PositionIndexer {
 public:
  [[nodiscard]] static std::optional<CanonicalPosition> Canonicalize(
      const Position& position);
  [[nodiscard]] static std::uint64_t RawIndex(
      const CanonicalPosition& position);
  [[nodiscard]] static std::optional<CanonicalPosition> FromRawIndex(
      MaterialClass material, std::uint64_t raw_index);
  [[nodiscard]] static std::uint64_t RawSize(const MaterialClass& material);
  [[nodiscard]] static std::optional<CanonicalizationTrace> Trace(
      const Position& position);
};

}  // namespace Tablebase
}  // namespace SimpleChessEngine
