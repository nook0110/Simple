#include "TablebasePosition.h"

#include <array>

namespace SimpleChessEngine::Tablebase {

std::optional<Position> PositionBuilder::Build(
    const CanonicalPosition& encoded) {
  return BuildImpl(encoded, nullptr);
}

PositionBuilder::BuildError PositionBuilder::Diagnose(
    const CanonicalPosition& encoded) {
  BuildError error = BuildError::kNone;
  (void)BuildImpl(encoded, &error);
  return error;
}

std::optional<Position> PositionBuilder::BuildImpl(
    const CanonicalPosition& encoded, BuildError* error) {
  const auto fail = [error](const BuildError value) -> std::optional<Position> {
    if (error) *error = value;
    return std::nullopt;
  };
  if (encoded.material.extra_count > 2) {
    return fail(BuildError::kInvalidMaterial);
  }
  const auto piece_count = encoded.material.PieceCount();
  for (std::uint8_t i = 0; i < piece_count; ++i) {
    if (!IsOk(encoded.squares[i])) return fail(BuildError::kInvalidSquare);
    for (std::uint8_t j = 0; j < i; ++j) {
      if (encoded.squares[i] == encoded.squares[j]) {
        return fail(BuildError::kOverlappingPieces);
      }
    }
  }
  if (KingDistance(encoded.squares[0], encoded.squares[1]) <= 1) {
    return fail(BuildError::kAdjacentKings);
  }

  Position position;
  position.PlacePiece(encoded.squares[0], Piece::kKing, Player::kWhite);
  position.PlacePiece(encoded.squares[1], Piece::kKing, Player::kBlack);
  for (std::uint8_t i = 0; i < encoded.material.extra_count; ++i) {
    const auto code = encoded.material.extras[i];
    if (code == PieceCode::kNone) return fail(BuildError::kInvalidPiece);
    const auto piece = DecodePiece(code);
    const auto square = encoded.squares[i + 2];
    const auto rank = GetCoordinates(square).second;
    if (piece == Piece::kPawn && (rank == 0 || rank == 7)) {
      return fail(BuildError::kPawnOnBackRank);
    }
    position.PlacePiece(square, piece, DecodeColor(code));
  }
  position.SetSideToMove(encoded.side_to_move);
  position.SetKingPositions({encoded.squares[0], encoded.squares[1]});
  position.SetRookPositions({{{0, 0}, {0, 0}}});
  position.Init();

  if (position.IsUnderCheck(Flip(encoded.side_to_move))) {
    return fail(BuildError::kSideNotToMoveInCheck);
  }
  if (error) *error = BuildError::kNone;
  return position;
}

bool PositionBuilder::IsCanonicalLegalRaw(const MaterialClass& material,
                                          const std::uint64_t raw_index) {
  const auto encoded = PositionIndexer::FromRawIndex(material, raw_index);
  if (!encoded) return false;
  const auto position = Build(*encoded);
  if (!position) return false;
  const auto canonical = PositionIndexer::Canonicalize(*position);
  return canonical && canonical->material == material &&
         PositionIndexer::RawIndex(*canonical) == raw_index;
}

void PositionBuilder::ClearEnPassant(Position& position) {
  position.SetEnCroissantSquare(std::nullopt);
}

}  // namespace SimpleChessEngine::Tablebase
