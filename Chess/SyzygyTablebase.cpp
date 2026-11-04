#include "SyzygyTablebase.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#include <tbprobe.h>

#include "Position.h"

namespace SimpleChessEngine::Tablebase {
namespace {

struct FathomPosition {
  std::uint64_t white{};
  std::uint64_t black{};
  std::uint64_t kings{};
  std::uint64_t queens{};
  std::uint64_t rooks{};
  std::uint64_t bishops{};
  std::uint64_t knights{};
  std::uint64_t pawns{};
  unsigned rule50{};
  unsigned castling{};
  unsigned ep{};
  bool white_to_move{};
};

[[nodiscard]] FathomPosition Encode(const Position& position) {
  FathomPosition result;
  result.white = static_cast<std::uint64_t>(
      position.GetPieces(Player::kWhite));
  result.black = static_cast<std::uint64_t>(
      position.GetPieces(Player::kBlack));
  result.kings = static_cast<std::uint64_t>(
      position.GetPiecesByType<Piece::kKing>(Player::kWhite) |
      position.GetPiecesByType<Piece::kKing>(Player::kBlack));
  result.queens = static_cast<std::uint64_t>(
      position.GetPiecesByType<Piece::kQueen>(Player::kWhite) |
      position.GetPiecesByType<Piece::kQueen>(Player::kBlack));
  result.rooks = static_cast<std::uint64_t>(
      position.GetPiecesByType<Piece::kRook>(Player::kWhite) |
      position.GetPiecesByType<Piece::kRook>(Player::kBlack));
  result.bishops = static_cast<std::uint64_t>(
      position.GetPiecesByType<Piece::kBishop>(Player::kWhite) |
      position.GetPiecesByType<Piece::kBishop>(Player::kBlack));
  result.knights = static_cast<std::uint64_t>(
      position.GetPiecesByType<Piece::kKnight>(Player::kWhite) |
      position.GetPiecesByType<Piece::kKnight>(Player::kBlack));
  result.pawns = static_cast<std::uint64_t>(
      position.GetPiecesByType<Piece::kPawn>(Player::kWhite) |
      position.GetPiecesByType<Piece::kPawn>(Player::kBlack));
  result.rule50 = static_cast<unsigned>(std::min<std::size_t>(
      position.GetHalfMoveClock(), std::numeric_limits<std::uint8_t>::max()));
  const auto& rights = position.GetCastlingRights();
  result.castling = rights[static_cast<std::size_t>(Player::kWhite)].any() ||
                            rights[static_cast<std::size_t>(Player::kBlack)].any()
                        ? 1U
                        : 0U;
  result.ep = position.GetEnCroissantSquare().has_value()
                  ? static_cast<unsigned>(*position.GetEnCroissantSquare())
                  : 0U;
  result.white_to_move = position.GetSideToMove() == Player::kWhite;
  return result;
}

[[nodiscard]] std::optional<Wdl> DecodeWdl(const unsigned value) {
  if (value > TB_WIN) return std::nullopt;
  return static_cast<Wdl>(static_cast<int>(value) - static_cast<int>(TB_DRAW));
}

[[nodiscard]] Piece DecodePromotion(const unsigned promotion) {
  switch (promotion) {
    case TB_PROMOTES_QUEEN:
      return Piece::kQueen;
    case TB_PROMOTES_ROOK:
      return Piece::kRook;
    case TB_PROMOTES_BISHOP:
      return Piece::kBishop;
    case TB_PROMOTES_KNIGHT:
      return Piece::kKnight;
    default:
      return Piece::kNone;
  }
}

[[nodiscard]] std::optional<Move> DecodeMove(const unsigned result) {
  if (result == TB_RESULT_FAILED || result == TB_RESULT_CHECKMATE ||
      result == TB_RESULT_STALEMATE) {
    return std::nullopt;
  }
  const auto from = static_cast<BitIndex>(TB_GET_FROM(result));
  const auto to = static_cast<BitIndex>(TB_GET_TO(result));
  const auto promotion = DecodePromotion(TB_GET_PROMOTES(result));
  if (promotion != Piece::kNone) {
    return Move::Make<MoveType::kPromotion>(from, to, promotion);
  }
  if (TB_GET_EP(result)) {
    return Move::Make<MoveType::kEnPassant>(from, to);
  }
  return Move{from, to};
}

[[nodiscard]] unsigned ProbeRoot(const FathomPosition& position) {
  return tb_probe_root(
      position.white, position.black, position.kings, position.queens,
      position.rooks, position.bishops, position.knights, position.pawns,
      position.rule50, position.castling, position.ep,
      position.white_to_move, nullptr);
}

}  // namespace

std::mutex Syzygy::initialization_mutex_;
std::mutex Syzygy::root_probe_mutex_;

std::shared_ptr<const Syzygy> Syzygy::Open(
    const std::filesystem::path& path) {
  std::lock_guard lock(initialization_mutex_);
  if (!tb_init(path.c_str()) || TB_LARGEST == 0) return {};
  return std::shared_ptr<const Syzygy>{new Syzygy{TB_LARGEST}};
}

void Syzygy::Disable() {
  std::lock_guard lock(initialization_mutex_);
  (void)tb_init("<empty>");
}

bool Syzygy::CanProbe(const Position& position) const {
  if (position.GetAllPieces().Count() > max_pieces_) return false;
  const auto& rights = position.GetCastlingRights();
  return rights[static_cast<std::size_t>(Player::kWhite)].none() &&
         rights[static_cast<std::size_t>(Player::kBlack)].none();
}

std::optional<ProbeResult> Syzygy::Probe(const Position& position) const {
  if (!CanProbe(position)) return std::nullopt;
  const auto encoded = Encode(position);
  std::lock_guard lock(root_probe_mutex_);
  const auto result = ProbeRoot(encoded);
  if (result == TB_RESULT_FAILED) return std::nullopt;
  if (result == TB_RESULT_CHECKMATE) {
    return ProbeResult{Wdl::kLoss, -1};
  }
  if (result == TB_RESULT_STALEMATE) {
    return ProbeResult{Wdl::kDraw, 0};
  }
  const auto wdl = DecodeWdl(TB_GET_WDL(result));
  if (!wdl) return std::nullopt;
  const auto sign = (static_cast<std::int8_t>(*wdl) > 0) -
                    (static_cast<std::int8_t>(*wdl) < 0);
  return ProbeResult{*wdl, static_cast<std::int16_t>(
                               sign * static_cast<int>(TB_GET_DTZ(result)))};
}

std::optional<Wdl> Syzygy::ProbeWdl(const Position& position) const {
  if (!CanProbe(position)) return std::nullopt;
  const auto encoded = Encode(position);
  const auto result = tb_probe_wdl(
      encoded.white, encoded.black, encoded.kings, encoded.queens,
      encoded.rooks, encoded.bishops, encoded.knights, encoded.pawns, 0,
      encoded.castling, encoded.ep, encoded.white_to_move);
  return result == TB_RESULT_FAILED ? std::nullopt : DecodeWdl(result);
}

std::optional<Move> Syzygy::RootMove(const Position& position) const {
  if (!CanProbe(position)) return std::nullopt;
  const auto encoded = Encode(position);
  std::lock_guard lock(root_probe_mutex_);
  return DecodeMove(ProbeRoot(encoded));
}

}  // namespace SimpleChessEngine::Tablebase
