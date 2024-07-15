#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <random>

#include "Attacks.h"
#include "Bitboard.h"
#include "Evaluation.h"
#include "Hasher.h"
#include "Move.h"
#include "PSQT.h"
#include "Piece.h"
#include "Player.h"
#include "Utility.h"

namespace SimpleChessEngine {
/**
 * \brief Class that represents a chess position.
 *
 * This class encapsulates all the data and functionality required to represent
 * and manipulate a chess position. It includes methods for making and undoing
 * moves, evaluating the position, checking game states such as check, and
 * managing game history for threefold repetition detection.
 *
 * \author nook0110
 * \author alfoos
 */
class Position {
 public:
  friend class PositionFactory;
  /**
   * \brief Struct to hold evaluation data for the position.
   *
   * This struct contains various metrics used in the evaluation of a chess
   * position, such as material count and piece-square table scores.
   */
  struct EvaluationData {
    bool operator==(const EvaluationData&) const = default;

    Eval non_pawn_material{};  //!< Total material value excluding pawns.
    std::array<TaperedEval, kColors>
        material{};  //!< Material value for each color.
    std::array<TaperedEval, kColors>
        psqt{};  //!< Piece-square table scores for each color.
  };

  /**
   * \brief Struct to hold data that cannot be reversed after a move.
   *
   * This struct contains information that changes irreversibly after a move,
   * such as en passant square, castling rights, and pinning information.
   */
  struct IrreversibleData {
    std::optional<BitIndex>
        en_croissant_square{};  //!< Optional square for en passant.

    std::array<std::bitset<2>, kColors>
        castling_rights{};  //!< Castling rights for each color.

    std::array<Bitboard, kColors>
        pinners{};  //!< Pieces that are pinning opponent's pieces.
    std::array<Bitboard, kColors>
        blockers{};  //!< Pieces that are blocking attacks on the king.

    bool operator==(const IrreversibleData& other) const {
      return std::tie(en_croissant_square, castling_rights) ==
             std::tie(other.en_croissant_square, other.castling_rights);
    }
  };

  /**
   * \brief Struct to manage the game's move history.
   *
   * This struct is responsible for tracking the history of moves made during
   * the game, which is used for detecting threefold repetitions.
   */
  struct GameHistory {
    static constexpr size_t kHistorySize =
        1024;  //!< Maximum size of the game history.

    GameHistory() {
      history.reserve(kHistorySize);
      last_reset.reserve(kHistorySize + 1);
      last_reset.push_back(0);
    }

    size_t Count(const Hash hash) const {
      size_t result = 0;
      size_t parity_shift =
          (history.size() ^ last_reset[history.size()] ^ 1) % 2;
      for (size_t i = last_reset[history.size()] + parity_shift;
           i < history.size(); i += 2) {
        if (history[i] == hash) {
          ++result;
        }
      }
      return result;
    }

    void Push(const Hash hash, const bool reset) {
      last_reset.push_back(reset ? history.size() : last_reset.back());
      history.push_back(hash);
    }

    void Pop() {
      history.pop_back();
      last_reset.pop_back();
    }

    std::vector<Hash>
        history{};  //!< Vector of hashes representing the game history.
    std::vector<size_t>
        last_reset{};  //!< Indices where the history was last reset.
  };

  [[nodiscard]] Eval Evaluate() const;

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const Move& move);

  /**
   * \brief Undoes given move.
   *
   * \param move Move to undo.
   */
  void UndoMove(const Move& move, const IrreversibleData& data);

  [[nodiscard]] bool CanCastle(
      const Castling::CastlingSide castling_side) const;

  /**
   * \brief Gets hash of the position.
   *
   * \return Hash of the position.
   */
  [[nodiscard]] Hash GetHash() const { return hash_; }

  /**
   * \brief Gets all pieces on the board.
   *
   * \return Bitboard with all pieces on the board.
   */
  [[nodiscard]] Bitboard GetAllPieces() const;

  /**
   * \brief Gets all pieces of given player.
   *
   * \param player Player to get pieces of.
   *
   * \return Bitboard with all pieces of given player.
   */
  [[nodiscard]] const Bitboard& GetPieces(Player player) const;

  template <Piece piece>
  [[nodiscard]] Bitboard GetPiecesByType(Player player) const;

  /**
   * \brief Gets piece on given square.
   *
   * \param index Square to get piece from.
   *
   * \return Piece on given square.
   */
  [[nodiscard]] Piece GetPiece(BitIndex index) const;

  /**
   * \brief Gets side to move.
   *
   * \return Side to move.
   */
  [[nodiscard]] Player GetSideToMove() const { return side_to_move_; }

  /**
   * \brief Sets side to move.
   *
   * \param player Player whose's side to move.
   */
  void SetSideToMove(const Player player) { side_to_move_ = player; }

  [[nodiscard]] Bitboard GetAllPawnAttacks(Player player) const;

  [[nodiscard]] BitIndex GetKingSquare(Player player) const;

  [[nodiscard]] BitIndex GetCastlingRookSquare(
      Player player, Castling::CastlingSide side) const;

  [[nodiscard]] Bitboard Attackers(BitIndex square,
                                   Bitboard transparent = kEmptyBoard) const;

  void ComputePins(Player us);

  [[nodiscard]] bool IsUnderAttack(BitIndex square, Player us,
                                   Bitboard transparent = kEmptyBoard) const;

  /**
   * \brief Checks if current player is under check.
   *
   * \return True if current player is under check, false otherwise.
   */
  [[nodiscard]] bool IsUnderCheck() const;

  /**
   * \brief Checks if given player is under check.
   *
   * \return True if given player is under check, false otherwise.
   */
  [[nodiscard]] bool IsUnderCheck(Player player) const;

  [[nodiscard]] bool DetectRepetition() const {
    if (history_stack_.history.size() < 3) {
      return false;
    }
    return history_stack_.Count(hash_) >= 3;
  }

  [[nodiscard]] const std::optional<BitIndex>& GetEnCroissantSquare() const;

  [[nodiscard]] const std::array<std::bitset<2>, kColors>& GetCastlingRights()
      const;

  template <Piece piece>
  [[nodiscard]] Bitboard GetCastlingSquares(Castling::CastlingSide side) const;

  [[nodiscard]] IrreversibleData GetIrreversibleData() const;
  /**
   * \brief Default operator==()
   *
   * \param other Other position to compare with.
   *
   * \return True if positions are the same, false otherwise.
   */
  bool operator==(const Position& other) const {
    return std::tie(hash_, side_to_move_, pieces_by_type_, pieces_by_color_,
                    irreversible_data_) ==
           std::tie(other.hash_, other.side_to_move_, other.pieces_by_type_,
                    other.pieces_by_color_, other.irreversible_data_);
  }

 private:
  /**
   * \brief Places a piece with a color on a chosen square.
   *
   * \param square Square to place piece.
   * \param piece Piece to place.
   * \param color Color of the piece.
   *
   */
  void PlacePiece(const BitIndex square, const Piece piece, const Player color);

  /**
   * \brief Removes a piece from a chosen square.
   *
   * \param square Square to remove piece from.
   * \param color Color of the piece.
   *
   */
  void RemovePiece(const BitIndex square, const Player color);

  void MovePiece(const BitIndex from, const BitIndex to, const Player color);

  void DoMove(const DefaultMove& move);
  void DoMove(const PawnPush& move);
  void DoMove(const DoublePush& move);
  void DoMove(const EnCroissant& move);
  void DoMove(const Promotion& move);
  void DoMove(const Castling& move);
  void UndoMove(const DefaultMove& move);
  void UndoMove(const PawnPush& move);
  void UndoMove(const DoublePush& move);
  void UndoMove(const EnCroissant& move);
  void UndoMove(const Promotion& move);
  void UndoMove(const Castling& move);

  void SetCastlingRights(const std::array<std::bitset<2>, 2>& castling_rights);

  void SetKingPositions(const std::array<BitIndex, 2>& king_position);

  void SetRookPositions(
      const std::array<std::array<BitIndex, 2>, kColors>& rook_positions);

  void SetCastlingSquares(
      const std::array<std::array<Bitboard, 2>, kColors>& cs_king,
      const std::array<std::array<Bitboard, 2>, kColors>& cs_rook);

  [[nodiscard]] Eval EstimatePiece(Piece piece) const;

  [[nodiscard]] bool StaticExchangeEvaluation(const Move& move,
                                              Eval threshold) const;

  EvaluationData evaluation_data_;
  IrreversibleData irreversible_data_;
  GameHistory history_stack_ = {};

  Player side_to_move_{};  //!< Whose side to move.

  std::array<Bitboard, kPieceTypes>
      pieces_by_type_{};  //!< Bitboard of pieces of certain type
  std::array<Bitboard, kColors>
      pieces_by_color_{};  //!< Bitboard of pieces for each player

  std::array<Piece, kBoardArea> board_{};  //!< Current position of pieces

  std::array<BitIndex, kColors> king_position_{};
  std::array<std::array<BitIndex, 2>, kColors>
      rook_positions_{};  // doesn't need to be updated

  std::array<std::array<Bitboard, 2>, kColors> castling_squares_for_king_{};
  std::array<std::array<Bitboard, 2>, kColors> castling_squares_for_rook_{};

  const static Hasher hasher_;
  Hash hash_{};
};

inline void Position::SetRookPositions(
    const std::array<std::array<BitIndex, 2>, kColors>& rook_positions) {
  rook_positions_ = rook_positions;
}

inline void Position::SetCastlingSquares(
    const std::array<std::array<Bitboard, 2>, kColors>& cs_king,
    const std::array<std::array<Bitboard, 2>, kColors>& cs_rook) {
  castling_squares_for_king_ = cs_king;
  castling_squares_for_rook_ = cs_rook;
}

inline Bitboard Position::GetAllPieces() const {
  return pieces_by_color_[static_cast<size_t>(Player::kWhite)] |
         pieces_by_color_[static_cast<size_t>(Player::kBlack)];
}

inline const Bitboard& Position::GetPieces(const Player player) const {
  return pieces_by_color_[static_cast<size_t>(player)];
}

template <Piece piece>
Bitboard Position::GetPiecesByType(const Player player) const {
  return pieces_by_color_[static_cast<size_t>(player)] &
         pieces_by_type_[static_cast<size_t>(piece)];
}

template <Piece piece>
Bitboard Position::GetCastlingSquares(Castling::CastlingSide side) const {
  static_assert(piece == Piece::kRook || piece == Piece::kKing);
  if constexpr (piece == Piece::kKing) {
    return castling_squares_for_king_[static_cast<size_t>(side_to_move_)]
                                     [static_cast<size_t>(side)];
  }
  if constexpr (piece == Piece::kRook) {
    return castling_squares_for_rook_[static_cast<size_t>(side_to_move_)]
                                     [static_cast<size_t>(side)];
  }
  assert(false);
  return {};
}

inline Bitboard Position::GetAllPawnAttacks(const Player player) const {
  const auto us = static_cast<size_t>(player);
  const auto pawns = GetPiecesByType<Piece::kPawn>(player);
  return Shift(pawns, kPawnAttackDirections[us][0]) |
         Shift(pawns, kPawnAttackDirections[us][1]);
}

inline Piece Position::GetPiece(const BitIndex index) const {
  assert(IsOk(index));
  return board_[index];
}

inline BitIndex Position::GetKingSquare(const Player player) const {
  return king_position_[static_cast<size_t>(player)];
}

inline BitIndex Position::GetCastlingRookSquare(
    Player player, Castling::CastlingSide side) const {
  return rook_positions_[static_cast<size_t>(player)]
                        [static_cast<size_t>(side)];
}

inline bool Position::IsUnderAttack(const BitIndex square, const Player us,
                                    const Bitboard transparent) const {
  assert(IsOk(square));
  return (Attackers(square, transparent) &
          pieces_by_color_[static_cast<size_t>(Flip(us))])
      .Any();
}

inline bool Position::IsUnderCheck() const {
  return IsUnderCheck(side_to_move_);
}

inline bool Position::IsUnderCheck(const Player player) const {
  return IsUnderAttack(GetKingSquare(player), player);
}

inline Eval Position::EstimatePiece(const Piece piece) const {
  const auto piece_type_idx = static_cast<size_t>(piece);
  return kPieceValues[piece_type_idx](evaluation_data_.non_pawn_material);
}

inline bool Position::StaticExchangeEvaluation(const Move& move,
                                               const Eval threshold) const {
  const auto us = side_to_move_;
  const auto them = Flip(us);

  const auto [from, to, captured_piece] = GetMoveData(move);

  const auto promotion_or = std::get_if<Promotion>(&move);

  Piece next_victim =
      !promotion_or ? GetPiece(from) : promotion_or->promoted_to;

  Eval balance = EstimatePiece(captured_piece);

  if (promotion_or) {
    balance +=
        EstimatePiece(promotion_or->promoted_to) - EstimatePiece(Piece::kPawn);
  } else if (std::holds_alternative<EnCroissant>(move)) {
    balance = EstimatePiece(Piece::kPawn);
  }

  balance -= threshold;

  if (balance < 0) {
    return false;
  }

  balance -= EstimatePiece(next_victim);

  if (balance >= 0) {
    return true;
  }

  Bitboard occupancy =
      GetAllPieces() ^ GetBitboardOfSquare(from) ^ GetBitboardOfSquare(to);

  [[unlikely]] if (std::holds_alternative<EnCroissant>(move)) {
    occupancy ^= Shift(GetBitboardOfSquare(to),
                       kPawnMoveDirection[static_cast<size_t>(them)]);
  }

  Bitboard attackers = Attackers(to, ~occupancy) & occupancy;

  auto color = them;

  auto diagonal_xray = pieces_by_type_[static_cast<size_t>(Piece::kBishop)] |
                       pieces_by_type_[static_cast<size_t>(Piece::kQueen)];

  auto straight_xray = pieces_by_type_[static_cast<size_t>(Piece::kRook)] |
                       pieces_by_type_[static_cast<size_t>(Piece::kQueen)];

  for (;;) {
    Bitboard our_attackers = attackers & GetPieces(color);
    if (our_attackers.None()) {
      break;
    }

    Bitboard attacker_bitboard;

    next_victim = Piece::kKing;

    for (size_t attacker_idx = static_cast<size_t>(Piece::kPawn);
         attacker_idx <= static_cast<size_t>(Piece::kQueen); ++attacker_idx) {
      if (attacker_bitboard = our_attackers & pieces_by_type_[attacker_idx];
          attacker_bitboard.Any()) {
        next_victim = static_cast<Piece>(attacker_idx);
        break;
      }
    }

    attacker_bitboard &= -attacker_bitboard;

    occupancy ^= attacker_bitboard;

    if (IsDiagonalAttacker(next_victim)) {
      attackers |= AttackTable<Piece::kBishop>::GetAttackMap(to, occupancy) &
                   diagonal_xray;
    }

    if (IsStraightAttacker(next_victim)) {
      attackers |= AttackTable<Piece::kRook>::GetAttackMap(to, occupancy) &
                   straight_xray;
    }

    attackers &= occupancy;

    color = Flip(color);

    balance = -balance - 1 - EstimatePiece(next_victim);

    if (balance >= 0) {
      if (next_victim == Piece::kKing && (attackers & GetPieces(color)).Any()) {
        color = Flip(color);
      }
      break;
    }
  }

  return color != GetSideToMove();
}

inline const std::optional<BitIndex>& Position::GetEnCroissantSquare() const {
  return irreversible_data_.en_croissant_square;
}

inline const std::array<std::bitset<2>, kColors>& Position::GetCastlingRights()
    const {
  return irreversible_data_.castling_rights;
}

inline Bitboard Position::Attackers(const BitIndex square,
                                    const Bitboard transparent) const {
  assert(IsOk(square));
  const Bitboard occupancy = GetAllPieces() & ~transparent;
  return GetPawnAttacks(square, Player::kWhite) &
             GetPiecesByType<Piece::kPawn>(Player::kBlack) |
         GetPawnAttacks(square, Player::kBlack) &
             GetPiecesByType<Piece::kPawn>(Player::kWhite) |
         AttackTable<Piece::kKnight>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kKnight)] |
         AttackTable<Piece::kBishop>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kBishop)] |
         AttackTable<Piece::kRook>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kRook)] |
         AttackTable<Piece::kQueen>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kQueen)] |
         AttackTable<Piece::kKing>::GetAttackMap(square, occupancy) &
             pieces_by_type_[static_cast<size_t>(Piece::kKing)];
}

inline void Position::ComputePins(const Player us) {
  const Player them = Flip(us);

  const auto us_idx = static_cast<size_t>(us);

  irreversible_data_.blockers[us_idx] = kEmptyBoard;
  irreversible_data_.pinners[us_idx] = kEmptyBoard;

  const BitIndex king_square = GetKingSquare(us);

  const Bitboard all_pieces = GetAllPieces();

  const Bitboard diagonal_blockers =
      AttackTable<Piece::kBishop>::GetAttackMap(king_square, all_pieces) &
      all_pieces;
  const Bitboard straight_blockers =
      AttackTable<Piece::kRook>::GetAttackMap(king_square, all_pieces) &
      all_pieces;

  Bitboard diagonal_pinners = AttackTable<Piece::kBishop>::GetAttackMap(
                                  king_square, all_pieces ^ diagonal_blockers) &
                              (GetPiecesByType<Piece::kBishop>(them) |
                               GetPiecesByType<Piece::kQueen>(them)) &
                              ~diagonal_blockers;
  Bitboard straight_pinners = AttackTable<Piece::kRook>::GetAttackMap(
                                  king_square, all_pieces ^ straight_blockers) &
                              (GetPiecesByType<Piece::kRook>(them) |
                               GetPiecesByType<Piece::kQueen>(them)) &
                              ~straight_blockers;

  irreversible_data_.pinners[us_idx] = diagonal_pinners | straight_pinners;

  Bitboard& blockers = irreversible_data_.blockers[us_idx];
  while (diagonal_pinners.Any()) {
    const BitIndex square = diagonal_pinners.PopFirstBit();

    const auto blockers_between =
        bishop_between[square][king_square] & diagonal_blockers;
    assert(blockers_between.Any() && !blockers_between.MoreThanOne());
    blockers |= blockers_between;
  }
  while (straight_pinners.Any()) {
    const BitIndex square = straight_pinners.PopFirstBit();

    const auto blockers_between =
        rook_between[square][king_square] & straight_blockers;
    assert(blockers_between.Any() && !blockers_between.MoreThanOne());
    blockers |= blockers_between;
  }
}

inline Position::IrreversibleData Position::GetIrreversibleData() const {
  return irreversible_data_;
}
}  // namespace SimpleChessEngine