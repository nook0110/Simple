#include "Tablebase.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <tuple>
#include <utility>
#include <set>

#include "Position.h"

namespace SimpleChessEngine::Tablebase {
namespace {

struct LocatedPiece {
  PieceCode code;
  BitIndex square;
};

struct LocatedExtras {
  std::array<LocatedPiece, 2> pieces{};
  std::uint8_t count{};
  bool overflow{};
};

struct OrientedPosition {
  MaterialClass material;
  std::array<BitIndex, 4> squares{};
  Player side_to_move;
};

[[nodiscard]] BitIndex TransformSquare(const BitIndex square,
                                       const std::uint8_t symmetry) {
  const auto [file, rank] = GetCoordinates(square);
  switch (symmetry) {
    case 0:
      return GetSquareIndex(file, rank);
    case 1:
      return GetSquareIndex(7 - file, rank);
    case 2:
      return GetSquareIndex(file, 7 - rank);
    case 3:
      return GetSquareIndex(7 - file, 7 - rank);
    case 4:
      return GetSquareIndex(rank, file);
    case 5:
      return GetSquareIndex(7 - rank, file);
    case 6:
      return GetSquareIndex(rank, 7 - file);
    case 7:
      return GetSquareIndex(7 - rank, 7 - file);
    default:
      std::unreachable();
  }
}

void SortEqualExtraSquares(OrientedPosition& position) {
  if (position.material.extra_count == 2 &&
      position.material.extras[0] == position.material.extras[1] &&
      position.squares[2] > position.squares[3]) {
    std::swap(position.squares[2], position.squares[3]);
  }
}

[[nodiscard]] auto PositionKey(const OrientedPosition& position) {
  return std::tuple{position.squares,
                    static_cast<std::uint8_t>(position.side_to_move)};
}

[[nodiscard]] OrientedPosition ApplySymmetry(
    const OrientedPosition& position, const std::uint8_t symmetry) {
  OrientedPosition transformed = position;
  const auto piece_count = position.material.PieceCount();
  if (piece_count > transformed.squares.size()) std::unreachable();
  for (std::uint8_t i = 0; i < piece_count; ++i) {
    transformed.squares[i] = TransformSquare(position.squares[i], symmetry);
  }
  SortEqualExtraSquares(transformed);
  return transformed;
}

[[nodiscard]] OrientedPosition CanonicalGeometry(
    const OrientedPosition& position) {
  OrientedPosition best = position;
  const std::uint8_t symmetry_count = position.material.HasPawns() ? 2 : 8;
  for (std::uint8_t symmetry = 0; symmetry < symmetry_count; ++symmetry) {
    auto transformed = ApplySymmetry(position, symmetry);
    if (PositionKey(transformed) < PositionKey(best)) {
      best = transformed;
    }
  }
  return best;
}

[[nodiscard]] OrientedPosition BuildOrientation(
    const BitIndex white_king, const BitIndex black_king,
    const LocatedExtras& extras, const Player side_to_move,
    const bool swap_colors) {
  OrientedPosition result;
  result.material.extra_count = extras.count;
  for (std::uint8_t i = 0; i < extras.count; ++i) {
    result.material.extras[i] =
        swap_colors ? FlipColor(extras.pieces[i].code)
                    : extras.pieces[i].code;
    result.squares[i + 2] =
        swap_colors ? TransformSquare(extras.pieces[i].square, 2)
                    : extras.pieces[i].square;
  }
  if (extras.count == 2 &&
      std::tie(result.material.extras[1], result.squares[3]) <
          std::tie(result.material.extras[0], result.squares[2])) {
    std::swap(result.material.extras[0], result.material.extras[1]);
    std::swap(result.squares[2], result.squares[3]);
  }
  if (swap_colors) {
    result.squares[0] = TransformSquare(black_king, 2);
    result.squares[1] = TransformSquare(white_king, 2);
    result.side_to_move = Flip(side_to_move);
  } else {
    result.squares[0] = white_king;
    result.squares[1] = black_king;
    result.side_to_move = side_to_move;
  }
  SortEqualExtraSquares(result);
  return result;
}

[[nodiscard]] LocatedExtras CollectExtras(
    const Position& position) {
  LocatedExtras extras;
  auto pieces = position.GetAllPieces();
  const auto white = position.GetPieces(Player::kWhite);
  while (pieces.Any()) {
    const auto square = pieces.PopFirstBit();
    const auto piece = position.GetPieceAt(square);
    if (piece == Piece::kKing) continue;
    if (extras.count == extras.pieces.size()) {
      extras.overflow = true;
      return extras;
    }
    const auto color = white.Test(square) ? Player::kWhite : Player::kBlack;
    extras.pieces[extras.count++] = {EncodePiece(piece, color), square};
  }
  return extras;
}

}  // namespace

bool MaterialClass::HasPawns() const {
  return std::ranges::any_of(extras, [](const PieceCode code) {
    return code == PieceCode::kWhitePawn || code == PieceCode::kBlackPawn;
  });
}

PieceCode EncodePiece(const Piece piece, const Player color) {
  assert(piece >= Piece::kPawn && piece <= Piece::kQueen);
  const auto offset = color == Player::kWhite ? 0 : 5;
  return static_cast<PieceCode>(offset + static_cast<std::uint8_t>(piece));
}

Piece DecodePiece(const PieceCode code) {
  assert(code != PieceCode::kNone);
  const auto value = static_cast<std::uint8_t>(code);
  return static_cast<Piece>((value - 1) % 5 + 1);
}

Player DecodeColor(const PieceCode code) {
  assert(code != PieceCode::kNone);
  return static_cast<std::uint8_t>(code) <=
                 static_cast<std::uint8_t>(PieceCode::kWhiteQueen)
             ? Player::kWhite
             : Player::kBlack;
}

PieceCode FlipColor(const PieceCode code) {
  if (code == PieceCode::kNone) return code;
  const auto value = static_cast<std::uint8_t>(code);
  return static_cast<PieceCode>(value <= 5 ? value + 5 : value - 5);
}

std::uint16_t MaterialKey(const MaterialClass& material) {
  return static_cast<std::uint16_t>(material.extra_count) << 8 |
         static_cast<std::uint16_t>(material.extras[0]) << 4 |
         static_cast<std::uint16_t>(material.extras[1]);
}

std::vector<MaterialClass> AllMaterialClasses() {
  std::set<MaterialClass> classes;
  classes.insert(MaterialClass{});
  const auto normalize = [](MaterialClass material) {
    MaterialClass flipped = material;
    for (std::uint8_t i = 0; i < material.extra_count; ++i) {
      flipped.extras[i] = FlipColor(material.extras[i]);
    }
    std::ranges::sort(flipped.extras.begin(),
                      flipped.extras.begin() + flipped.extra_count);
    return std::min(material, flipped);
  };

  for (std::uint8_t first = 1; first <= 10; ++first) {
    classes.insert(normalize(MaterialClass{
        {static_cast<PieceCode>(first), PieceCode::kNone}, 1}));
    for (std::uint8_t second = first; second <= 10; ++second) {
      classes.insert(normalize(MaterialClass{
          {static_cast<PieceCode>(first), static_cast<PieceCode>(second)},
          2}));
    }
  }
  return {classes.begin(), classes.end()};
}

std::optional<CanonicalPosition> PositionIndexer::Canonicalize(
    const Position& position) {
  if (position.GetEnCroissantSquare().has_value()) return std::nullopt;
  const auto& castling_rights = position.GetCastlingRights();
  if (castling_rights[0].any() || castling_rights[1].any()) {
    return std::nullopt;
  }
  if (position.GetPiecesByType<Piece::kKing>(Player::kWhite).Count() != 1 ||
      position.GetPiecesByType<Piece::kKing>(Player::kBlack).Count() != 1) {
    return std::nullopt;
  }

  auto extras = CollectExtras(position);
  if (extras.overflow) return std::nullopt;

  const auto white_king = position.GetKingSquare(Player::kWhite);
  const auto black_king = position.GetKingSquare(Player::kBlack);
  const auto original = BuildOrientation(white_king, black_king, extras,
                                         position.GetSideToMove(), false);
  const auto swapped = BuildOrientation(white_king, black_king, extras,
                                        position.GetSideToMove(), true);

  const auto canonical_material = std::min(original.material, swapped.material);
  std::optional<OrientedPosition> best;
  for (const auto& orientation : {original, swapped}) {
    if (orientation.material != canonical_material) continue;
    const auto candidate = CanonicalGeometry(orientation);
    if (!best || PositionKey(candidate) < PositionKey(*best)) {
      best = candidate;
    }
  }
  assert(best.has_value());
  return CanonicalPosition{best->material, best->squares, best->side_to_move};
}

std::optional<CanonicalizationTrace> PositionIndexer::Trace(
    const Position& position) {
  if (position.GetPiecesByType<Piece::kKing>(Player::kWhite).Count() != 1 ||
      position.GetPiecesByType<Piece::kKing>(Player::kBlack).Count() != 1) {
    return std::nullopt;
  }
  const auto extras = CollectExtras(position);
  if (extras.overflow) return std::nullopt;
  const auto white_king = position.GetKingSquare(Player::kWhite);
  const auto black_king = position.GetKingSquare(Player::kBlack);
  const auto original = BuildOrientation(white_king, black_king, extras,
                                         position.GetSideToMove(), false);
  const auto swapped = BuildOrientation(white_king, black_king, extras,
                                        position.GetSideToMove(), true);
  const auto original_geometry = CanonicalGeometry(original);
  const auto swapped_geometry = CanonicalGeometry(swapped);
  return CanonicalizationTrace{
      original.material,
      swapped.material,
      CanonicalPosition{original_geometry.material, original_geometry.squares,
                        original_geometry.side_to_move},
      CanonicalPosition{swapped_geometry.material, swapped_geometry.squares,
                        swapped_geometry.side_to_move}};
}

std::uint64_t PositionIndexer::RawIndex(
    const CanonicalPosition& position) {
  std::uint64_t rank = 0;
  const auto piece_count = position.material.PieceCount();
  for (std::uint8_t i = 0; i < piece_count; ++i) {
    const auto square = position.squares[i];
    assert(IsOk(square));
    std::uint8_t ordinal = static_cast<std::uint8_t>(square);
    for (std::uint8_t previous = 0; previous < i; ++previous) {
      assert(position.squares[previous] != square);
      ordinal -= position.squares[previous] < square;
    }
    rank = rank * (kBoardArea - i) + ordinal;
  }
  return rank * 2 + static_cast<std::uint8_t>(position.side_to_move);
}

std::optional<CanonicalPosition> PositionIndexer::FromRawIndex(
    const MaterialClass material, const std::uint64_t raw_index) {
  if (raw_index >= RawSize(material)) return std::nullopt;
  auto permutation_rank = raw_index / 2;
  const auto piece_count = material.PieceCount();
  std::array<std::uint8_t, 4> digits{};
  for (std::uint8_t reverse = 0; reverse < piece_count; ++reverse) {
    const auto i = piece_count - reverse - 1;
    const auto radix = kBoardArea - i;
    digits[i] = permutation_rank % radix;
    permutation_rank /= radix;
  }

  std::array<BitIndex, kBoardArea> available{};
  for (BitIndex square = 0; square < static_cast<BitIndex>(kBoardArea);
       ++square) {
    available[square] = square;
  }
  CanonicalPosition result{material};
  for (std::uint8_t i = 0; i < piece_count; ++i) {
    result.squares[i] = available[digits[i]];
    for (std::size_t index = digits[i]; index + 1 < kBoardArea - i; ++index) {
      available[index] = available[index + 1];
    }
  }
  result.side_to_move =
      raw_index % 2 == 0 ? Player::kWhite : Player::kBlack;
  return result;
}

std::uint64_t PositionIndexer::RawSize(const MaterialClass& material) {
  std::uint64_t size = 2;
  for (std::uint8_t i = 0; i < material.PieceCount(); ++i) {
    size *= kBoardArea - i;
  }
  return size;
}

}  // namespace SimpleChessEngine::Tablebase
