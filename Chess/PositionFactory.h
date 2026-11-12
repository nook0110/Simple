#pragma once
#include <algorithm>
#include <array>
#include <bitset>
#include <charconv>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "BitBoard.h"
#include "Position.h"
#include "StreamUtility.h"

namespace SimpleChessEngine {
inline constexpr std::string_view kStartFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct PositionFactory {
  [[nodiscard]] std::optional<Position> TryParse(std::string_view fen) const;
  [[nodiscard]] Position operator()(std::string_view fen = kStartFen) const;

 private:
  static constexpr File kFiles = 8;
  static constexpr Rank kRanks = 8;
  static constexpr int kMaxPiecesPerSide = 16;
  static constexpr int kMaxPawnsPerSide = 8;

  using CastlingRightsBySide = std::array<std::bitset<2>, kColors>;
  using RookSquares = std::array<std::array<std::optional<BitIndex>, 2>, kColors>;

  struct Fields {
    std::string board;
    std::string side_to_move;
    std::string castling;
    std::string en_passant = "-";
    std::string halfmove_clock = "0";
    std::string fullmove_number = "1";
  };

  struct Placement {
    std::array<BitIndex, kColors> kings{};
    std::array<std::array<bool, kFiles>, kColors> back_rank_rooks{};
  };

  [[nodiscard]] static constexpr Rank BackRank(Player color) {
    return color == Player::kWhite ? 0 : kRanks - 1;
  }

  [[nodiscard]] static std::optional<Fields> Split(std::string_view fen);
  [[nodiscard]] static std::optional<Placement> PlacePieces(
      std::string_view board, Position &position);
  [[nodiscard]] static std::optional<Player> ParseSideToMove(
      std::string_view text);
  [[nodiscard]] static RookSquares FindCastlingRooks(
      const Placement &placement);
  [[nodiscard]] static std::optional<CastlingRightsBySide> ParseCastlingRights(
      std::string_view text, const Placement &placement,
      const RookSquares &rooks);
  [[nodiscard]] static std::optional<BitIndex> ParseSquare(
      std::string_view text);
  [[nodiscard]] static bool IsEnPassantTarget(const Position &position,
                                              BitIndex square);
  [[nodiscard]] static std::optional<std::size_t> ParseNumber(
      std::string_view text);
};

struct FenFactory {
  std::string operator()(const Position &position);
};

inline std::optional<PositionFactory::Fields> PositionFactory::Split(
    std::string_view fen) {
  std::istringstream stream{std::string{fen}};
  const std::vector<std::string> tokens{
      std::istream_iterator<std::string>{stream},
      std::istream_iterator<std::string>{}};
  if (tokens.size() < 3 || tokens.size() > 6) return std::nullopt;

  Fields fields{tokens[0], tokens[1], tokens[2]};
  if (tokens.size() > 3) fields.en_passant = tokens[3];
  if (tokens.size() > 4) fields.halfmove_clock = tokens[4];
  if (tokens.size() > 5) fields.fullmove_number = tokens[5];
  return fields;
}

inline std::optional<PositionFactory::Placement> PositionFactory::PlacePieces(
    std::string_view board, Position &position) {
  Placement placement;
  std::array<int, kColors> kings{};
  std::array<int, kColors> pawns{};
  std::array<int, kColors> pieces{};

  Rank rank = kRanks - 1;
  File file = 0;
  for (const char symbol : board) {
    if (symbol == '/') {
      if (file != kFiles || rank == 0) return std::nullopt;
      --rank;
      file = 0;
      continue;
    }
    if (symbol >= '1' && symbol <= '8') {
      file += symbol - '0';
      if (file > kFiles) return std::nullopt;
      continue;
    }

    const auto entry = kCharToPiece.find(symbol);
    if (entry == kCharToPiece.end() || file == kFiles) return std::nullopt;
    const auto [piece, color] = entry->second;
    const auto us = static_cast<size_t>(color);
    const auto square = GetSquareIndex(file, rank);

    if (++pieces[us] > kMaxPiecesPerSide) return std::nullopt;
    if (piece == Piece::kKing) {
      if (++kings[us] > 1) return std::nullopt;
      placement.kings[us] = square;
    }
    if (piece == Piece::kPawn &&
        (rank == 0 || rank == kRanks - 1 || ++pawns[us] > kMaxPawnsPerSide)) {
      return std::nullopt;
    }
    if (piece == Piece::kRook && rank == BackRank(color)) {
      placement.back_rank_rooks[us][file] = true;
    }

    position.PlacePiece(square, piece, color);
    ++file;
  }

  if (rank != 0 || file != kFiles || kings != std::array{1, 1}) {
    return std::nullopt;
  }
  return placement;
}

inline std::optional<Player> PositionFactory::ParseSideToMove(
    std::string_view text) {
  if (text == "w") return Player::kWhite;
  if (text == "b") return Player::kBlack;
  return std::nullopt;
}

inline PositionFactory::RookSquares PositionFactory::FindCastlingRooks(
    const Placement &placement) {
  RookSquares rooks{};
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    const auto us = static_cast<size_t>(color);
    const auto &back_rank_rooks = placement.back_rank_rooks[us];
    const auto king_file = static_cast<File>(placement.kings[us] % kFiles);
    for (File file = 0; file < king_file; ++file) {
      if (back_rank_rooks[file]) {
        rooks[us][static_cast<size_t>(CastlingSide::k000)] =
            GetSquareIndex(file, BackRank(color));
        break;
      }
    }
    for (File file = kFiles - 1; file > king_file; --file) {
      if (back_rank_rooks[file]) {
        rooks[us][static_cast<size_t>(CastlingSide::k00)] =
            GetSquareIndex(file, BackRank(color));
        break;
      }
    }
  }
  return rooks;
}

inline std::optional<PositionFactory::CastlingRightsBySide>
PositionFactory::ParseCastlingRights(std::string_view text,
                                     const Placement &placement,
                                     const RookSquares &rooks) {
  CastlingRightsBySide rights;
  if (text == "-") return rights;

  constexpr std::string_view kSymbols = "KQkq";
  const auto is_shredder_file = [](const char symbol) {
    return (symbol >= 'A' && symbol <= 'H') || (symbol >= 'a' && symbol <= 'h');
  };
  for (const char symbol : text) {
    const auto index = kSymbols.find(symbol);
    if (index == std::string_view::npos) {
      if (is_shredder_file(symbol)) continue;
      return std::nullopt;
    }

    const auto color = index < 2 ? Player::kWhite : Player::kBlack;
    const auto side = index % 2 == 0 ? CastlingSide::k00 : CastlingSide::k000;
    const auto us = static_cast<size_t>(color);
    const auto bit = static_cast<size_t>(side);
    const bool king_on_back_rank =
        placement.kings[us] / kFiles == BackRank(color);
    if (rights[us].test(bit) || !king_on_back_rank || !rooks[us][bit]) {
      return std::nullopt;
    }
    rights[us].set(bit);
  }
  return rights;
}

inline std::optional<BitIndex> PositionFactory::ParseSquare(
    std::string_view text) {
  if (text.size() != 2 || text[0] < 'a' || text[0] > 'h' || text[1] < '1' ||
      text[1] > '8') {
    return std::nullopt;
  }
  return GetSquareIndex(static_cast<File>(text[0] - 'a'),
                        static_cast<Rank>(text[1] - '1'));
}

inline bool PositionFactory::IsEnPassantTarget(const Position &position,
                                               const BitIndex square) {
  const auto us = position.GetSideToMove();
  const auto them = Flip(us);
  const Rank target_rank = us == Player::kWhite ? kRanks - 3 : 2;
  if (square / kFiles != target_rank) return false;

  const auto forward = static_cast<BitIndex>(us == Player::kWhite ? kFiles : -kFiles);
  const auto pushed_pawn = static_cast<BitIndex>(square - forward);
  const auto pawn_origin = static_cast<BitIndex>(square + forward);
  return position.GetPieceAt(pushed_pawn) == Piece::kPawn &&
         (position.GetPieces(them) & SingleSquare(pushed_pawn)).Any() &&
         position.GetPieceAt(square) == Piece::kNone &&
         position.GetPieceAt(pawn_origin) == Piece::kNone;
}

inline std::optional<std::size_t> PositionFactory::ParseNumber(
    std::string_view text) {
  std::size_t number = 0;
  const auto end = text.data() + text.size();
  const auto [parsed_end, error] = std::from_chars(text.data(), end, number);
  if (error != std::errc{} || parsed_end != end) return std::nullopt;
  return number;
}

inline std::optional<Position> PositionFactory::TryParse(
    std::string_view fen) const {
  const auto fields = Split(fen);
  if (!fields) return std::nullopt;

  Position position;
  const auto placement = PlacePieces(fields->board, position);
  if (!placement) return std::nullopt;

  const auto side_to_move = ParseSideToMove(fields->side_to_move);
  if (!side_to_move) return std::nullopt;
  position.SetSideToMove(*side_to_move);

  const auto rooks = FindCastlingRooks(*placement);
  const auto castling_rights =
      ParseCastlingRights(fields->castling, *placement, rooks);
  if (!castling_rights) return std::nullopt;
  position.SetCastlingRights(*castling_rights);

  if (fields->en_passant != "-") {
    const auto square = ParseSquare(fields->en_passant);
    if (!square || !IsEnPassantTarget(position, *square)) return std::nullopt;
    position.SetEnCroissantSquare(*square);
  }

  const auto halfmove_clock = ParseNumber(fields->halfmove_clock);
  const auto fullmove_number = ParseNumber(fields->fullmove_number);
  if (!halfmove_clock || !fullmove_number) return std::nullopt;

  const auto &kings = placement->kings;
  std::array<std::array<BitIndex, 2>, kColors> rook_squares{};
  for (size_t us = 0; us < kColors; ++us) {
    for (size_t side = 0; side < 2; ++side) {
      rook_squares[us][side] = rooks[us][side].value_or(BitIndex{0});
    }
  }
  position.SetKingPositions(kings);
  position.SetRookPositions(rook_squares);
  std::array<std::array<Bitboard, 2>, kColors> cs_king = {
      {{RookBetween(kings[0], kKingCastlingDestination[0][0]),
        RookBetween(kings[0], kKingCastlingDestination[0][1])},
       {RookBetween(kings[1], kKingCastlingDestination[1][0]),
        RookBetween(kings[1], kKingCastlingDestination[1][1])}}};
  std::array<std::array<Bitboard, 2>, kColors> cs_rook = {
      {{RookBetween(rook_squares[0][0], kRookCastlingDestination[0][0]),
        RookBetween(rook_squares[0][1], kRookCastlingDestination[0][1])},
       {RookBetween(rook_squares[1][0], kRookCastlingDestination[1][0]),
        RookBetween(rook_squares[1][1], kRookCastlingDestination[1][1])}}};
  position.SetCastlingSquares(cs_king, cs_rook);

  position.Init(*halfmove_clock, std::max<std::size_t>(*fullmove_number, 1));
  return position;
}

inline Position PositionFactory::operator()(std::string_view fen) const {
  return TryParse(fen).value();
}

inline std::string SimpleChessEngine::FenFactory::operator()(
    const Position &position) {
  std::ostringstream ss;
  for (Rank rank = 7; rank >= 0; --rank) {
    size_t empty = 0;
    for (File file = 0; file < 8; ++file) {
      if ((position.GetAllPieces() & SingleSquare(GetSquareIndex(file, rank)))
              .None()) {
        ++empty;
        continue;
      }

      if (empty) {
        ss << empty;
        empty = 0;
      }
      const auto piece_type = position.GetPieceAt(GetSquareIndex(file, rank));
      const auto is_white = (position.GetPieces(Player::kWhite) &
                             SingleSquare(GetSquareIndex(file, rank)))
                                .Any();
      ss << GetPieceChar(piece_type,
                         is_white ? Player::kWhite : Player::kBlack);
    }
    if (empty) ss << empty;
    if (rank > 0) ss << '/';
  }

  ss << (position.GetSideToMove() == Player::kWhite ? " w " : " b ");

  const auto castling_rights = position.GetCastlingRights();

  if (castling_rights[static_cast<size_t>(Player::kWhite)].test(
          static_cast<size_t>(CastlingSide::k00))) {
    ss << 'K';
  }
  if (castling_rights[static_cast<size_t>(Player::kWhite)].test(
          static_cast<size_t>(CastlingSide::k000))) {
    ss << 'Q';
  }
  if (castling_rights[static_cast<size_t>(Player::kBlack)].test(
          static_cast<size_t>(CastlingSide::k00))) {
    ss << 'k';
  }
  if (castling_rights[static_cast<size_t>(Player::kBlack)].test(
          static_cast<size_t>(CastlingSide::k000))) {
    ss << 'q';
  }

  if ((castling_rights[static_cast<size_t>(Player::kWhite)] |
       castling_rights[static_cast<size_t>(Player::kBlack)])
          .none()) {
    ss << '-';
  }

  if (auto en_croissant_square = position.GetEnCroissantSquare()) {
    ss << " ";
    PrintCoordinates(GetCoordinates(*en_croissant_square), ss);
  } else {
    ss << " -";
  }

  ss << " " << position.GetHalfMoveClock() << " "
     << position.GetFullMoveNumber();

  return ss.str();
}
}  // namespace SimpleChessEngine
