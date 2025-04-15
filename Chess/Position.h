#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>

#include "BitBoard.h"
#include "Evaluation.h"
#include "Hasher.h"
#include "Move.h"
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
  friend struct PositionFactory;
  /**
   * \brief Struct to hold evaluation data for the position.
   *
   * This struct contains various metrics used in the evaluation of a chess
   * position, such as material count and piece-square table scores.
   */
  struct EvaluationData {
    bool operator==(const EvaluationData &) const = default;

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

    bool operator==(const IrreversibleData &other) const {
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

    size_t Count(const Hash hash, Depth depth) const;

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

  void Init() { history_stack_.Push(hash_, true); }

  [[nodiscard]] Eval Evaluate() const;

  /**
   * \brief Does given move.
   *
   * \param move Move to do.
   */
  void DoMove(const Move &move);

  /**
   * \brief Undoes given move.
   *
   * \param move Move to undo.
   */
  void UndoMove(const Move &move, const IrreversibleData &data);

  void DoMove(NullMove);
  void UndoMove(NullMove, const IrreversibleData &data);

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
  [[nodiscard]] const Bitboard &GetPieces(Player player) const;

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

  [[nodiscard]] bool DetectRepetition(Depth depth) const {
    if (history_stack_.history.size() < 3) {
      return false;
    }
    return history_stack_.Count(hash_, depth) >= 3;
  }

  [[nodiscard]] bool StaticExchangeEvaluation(const Move &move,
                                              Eval threshold) const;

  [[nodiscard]] const std::optional<BitIndex> &GetEnCroissantSquare() const;

  [[nodiscard]] const std::array<std::bitset<2>, kColors> &GetCastlingRights()
      const;

  template <Piece piece>
  [[nodiscard]] Bitboard GetCastlingSquares(Castling::CastlingSide side) const;

  [[nodiscard]] IrreversibleData GetIrreversibleData() const;

  size_t GetHalfMoveClock() const {
    return std::max<size_t>(1, history_stack_.history.size() -
                                   history_stack_.last_reset.back()) -
           1;
  }

  size_t GetFullMoveNumber() const {
    return history_stack_.history.size() / 2 + 1;
  }

  /**
   * \brief Default operator==()
   *
   * \param other Other position to compare with.
   *
   * \return True if positions are the same, false otherwise.
   */
  bool operator==(const Position &other) const {
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

  void DoMove(const DefaultMove &move);
  void DoMove(const PawnPush &move);
  void DoMove(const DoublePush &move);
  void DoMove(const EnCroissant &move);
  void DoMove(const Promotion &move);
  void DoMove(const Castling &move);
  void UndoMove(const DefaultMove &move);
  void UndoMove(const PawnPush &move);
  void UndoMove(const DoublePush &move);
  void UndoMove(const EnCroissant &move);
  void UndoMove(const Promotion &move);
  void UndoMove(const Castling &move);

  void SetCastlingRights(const std::array<std::bitset<2>, 2> &castling_rights);

  void SetKingPositions(const std::array<BitIndex, 2> &king_position);

  void SetRookPositions(
      const std::array<std::array<BitIndex, 2>, kColors> &rook_positions);

  void SetCastlingSquares(
      const std::array<std::array<Bitboard, 2>, kColors> &cs_king,
      const std::array<std::array<Bitboard, 2>, kColors> &cs_rook);

  [[nodiscard]] Eval EstimatePiece(Piece piece) const;

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

}  // namespace SimpleChessEngine