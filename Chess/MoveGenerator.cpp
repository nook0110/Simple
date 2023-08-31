#include "MoveGenerator.h"

#include <cassert>

#include "Attacks.h"

using namespace SimpleChessEngine;

template <bool only_attacks>
MoveGenerator::Moves MoveGenerator::GenerateMoves(Position& position) const
{
  // create moves
  Moves moves;

  // get all pieces
  auto pieces = position.GetPieces(position.GetSideToMove());

  // generate moves for each piece
  while (const auto from = pieces.GetFirstBit())
  {
    // get first piece
    pieces.Reset(*from);

    // generate moves for piece
    auto moves_for_piece = GenerateMovesForPiece<only_attacks>(position, *from);

    // add moves
    moves.splice(moves.end(), moves_for_piece);
  }

  // return moves
  return moves;
}

template MoveGenerator::Moves MoveGenerator::GenerateMoves<true>(
    Position& position) const;

template MoveGenerator::Moves MoveGenerator::GenerateMoves<false>(
    Position& position) const;

template <>
MoveGenerator::Moves
MoveGenerator::GenerateMovesFromSquare<Piece::kPawn, false>(
    Position& position, BitIndex from) const;

template <>
MoveGenerator::Moves MoveGenerator::GenerateMovesFromSquare<Piece::kPawn, true>(
    Position& position, BitIndex from) const;

template <bool only_attacks>
MoveGenerator::Moves MoveGenerator::GenerateMovesForPiece(
    Position& position, const BitIndex from) const
{
  // create moves, max amount moves 218
  Moves moves;

  // get piece and side to move
  const auto piece = position.GetPiece(from);

  // check if piece exists
  assert(position.GetAllPieces().Test(from));

  switch (piece)
  {
    case Piece::kPawn:
      moves =
          GenerateMovesFromSquare<Piece::kPawn, only_attacks>(position, from);
      break;
    case Piece::kKnight:
      // generate knight moves
      moves =
          GenerateMovesFromSquare<Piece::kKnight, only_attacks>(position, from);
      break;
    case Piece::kBishop:
      // generate bishop moves
      moves =
          GenerateMovesFromSquare<Piece::kBishop, only_attacks>(position, from);
      break;
    case Piece::kRook:
      // generate rook moves
      moves =
          GenerateMovesFromSquare<Piece::kRook, only_attacks>(position, from);
      break;
    case Piece::kQueen:
      // generate queen moves
      moves =
          GenerateMovesFromSquare<Piece::kQueen, only_attacks>(position, from);
      break;
    case Piece::kKing:
      // generate king moves
      moves =
          GenerateMovesFromSquare<Piece::kKing, only_attacks>(position, from);
      break;
    case Piece::kNone:
      assert(false);
      break;
    default:
      break;
  }

  // return moves
  return moves;
}

template <Piece piece, bool only_attacks>
MoveGenerator::Moves MoveGenerator::GenerateMovesFromSquare(
    Position& position, const BitIndex from) const
{
  assert(position.GetPiece(from) == piece);

  Moves moves;

  // get all squares that piece attacks
  const auto attacks =
      AttackTable<piece>::GetAttackMap(from, position.GetAllPieces());

  // get whose move is now
  const auto side_to_move = position.GetSideToMove();

  // get our pieces
  const auto& our_pieces = position.GetPieces(side_to_move);

  // remove moves into our pieces
  auto valid_moves = attacks & ~our_pieces;

  if constexpr (only_attacks)
  {
    const auto& enemy_piece = position.GetPieces(Flip(side_to_move));

    valid_moves &= enemy_piece;
  }

  while (const auto to = valid_moves.GetFirstBit())
  {
    valid_moves.Reset(to.value());

    if (auto move =
            DefaultMove{from, to.value(), position.GetPiece(to.value())};
        IsMoveValid(position, move))
    {
      moves.emplace_back(move);
    }
  }

  if constexpr (piece == Piece::kKing)
  {
    auto castling = GenerateCastling(position);
    moves.splice(moves.end(), castling);
  }

  // return moves
  return moves;
}

template <>
inline MoveGenerator::Moves
MoveGenerator::GenerateMovesFromSquare<Piece::kPawn, true>(
    Position& position, const BitIndex from) const
{
  auto side_to_move = position.GetSideToMove();

  if (static constexpr std::array kIsPromoting = {6, 1};
      GetCoordinates(from).second ==
      kIsPromoting[static_cast<size_t>(side_to_move)])
  {
    auto moves = GenerateMovesFromSquare<Piece::kPawn, false>(position, from);

    return moves;
  }

  return GenerateAttacksForPawn(position, from);
}

template <>
inline MoveGenerator::Moves
MoveGenerator::GenerateMovesFromSquare<Piece::kPawn, false>(
    Position& position, const BitIndex from) const
{
  Moves moves = GenerateAttacksForPawn(position, from);

  auto ordinary_moves = GenerateMovesForPawn(position, from);

  moves.splice(moves.end(), ordinary_moves);

  return ApplyPromotions(std::move(moves), position, from);
}

MoveGenerator::Moves MoveGenerator::GenerateCastling(
    const Position& position) const
{
  if (position.IsUnderCheck())
  {
    return {};
  }

  Moves moves;

  const auto side_to_move = position.GetSideToMove();

  const auto king_square = position.GetKingSquare(side_to_move);

  for (const auto castling_side : { Castling::CastlingSide::k00,
                                                Castling::CastlingSide::k000 })
  {
      if (position.CanCastle(castling_side))
      {
        const auto rook_square = position.GetCastlingRookSquare(side_to_move, castling_side);
        moves.emplace_back(Castling{castling_side, king_square, rook_square});
      }
    }

  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateAttacksForPawn(Position& position,
                                                           const BitIndex from)
{
  Moves moves;

  auto side_to_move = position.GetSideToMove();

  for (static constexpr std::array<std::array<Compass, 2>, kColors>
           kPawnAttackDirections = {{{Compass::kNorthWest, Compass::kNorthEast},
                                     {Compass::kSouthWest,
                                      Compass::kSouthEast}}};
       auto shift : kPawnAttackDirections[static_cast<size_t>(side_to_move)])
  {
    if (const BitIndex to = from + static_cast<BitIndex>(shift);
        IsShiftValid(to, from))
    {
      DefaultMove move{from, to, position.GetPiece(to)};

      if (position.GetPieces(Flip(side_to_move)).Test(to))
      {
        if (IsMoveValid(position, move)) moves.emplace_back(move);
      }
      else if (position.GetEnCroissantSquare() == to)
      {
        if (const EnCroissant en_croissant{from, to};
            IsMoveValid(position, en_croissant))
          moves.emplace_back(en_croissant);
      }
    }
  }

  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateMovesForPawn(Position& position,
                                                         const BitIndex from)
{
  Moves moves;
  static constexpr std::array kPawnDoublePushRank = {1, 6};
  static constexpr std::array kIsPromoting = {6, 1};

  const auto [file, rank] = GetCoordinates(from);
  const auto side_to_move = position.GetSideToMove();
  const auto push_direction =
      kPawnMoveDirection[static_cast<size_t>(side_to_move)];

  const auto to = Shift(from, push_direction);

  if (!position.GetPiece(to))
  {
    if (kIsPromoting[static_cast<size_t>(side_to_move)] ==
        GetCoordinates(from).second)
    {
      if (auto move = DefaultMove{from, to}; IsMoveValid(position, move))
      {
        moves.emplace_back(move);
      }
    }
    else
    {
      if (auto move = PawnPush{from, to}; IsMoveValid(position, move))
      {
        moves.emplace_back(move);
      }
    }
  }

  if (rank == kPawnDoublePushRank[static_cast<size_t>(side_to_move)] &&
      (position.GetAllPieces() &
       kDoubleMoveSpan[static_cast<size_t>(side_to_move)][file])
          .None())
  {
    if (auto move = DoublePush{from, Shift(to, push_direction)};
        IsMoveValid(position, move))
    {
      moves.emplace_back(move);
    }
  }

  return moves;
}

MoveGenerator::Moves MoveGenerator::ApplyPromotions(Moves moves,
                                                    const Position& position,
                                                    const BitIndex from)
{
  if (moves.empty()) return {};

  auto side_to_move = position.GetSideToMove();

  if (static constexpr std::array kIsPromoting = {6, 1};
      GetCoordinates(from).second ==
      kIsPromoting[static_cast<size_t>(side_to_move)])
  {
    static constexpr std::array kPiecesToPromoteTo = {
        Piece::kKnight, Piece::kBishop, Piece::kRook, Piece::kQueen};

    const auto end = moves.end();

    Moves other_promotions;

    for (auto it = moves.begin(); it != end; ++it)
    {
      auto promotion = static_cast<Promotion>(std::get<DefaultMove>(*it));
      promotion.promoted_to = kPiecesToPromoteTo.front();

      *it = promotion;

      for (size_t piece = 1; piece < kPiecesToPromoteTo.size(); piece++)
      {
        promotion.promoted_to = kPiecesToPromoteTo[piece];
        other_promotions.emplace_back(promotion);
      }
    }

    moves.splice(moves.end(), other_promotions);
  }

  return moves;
}