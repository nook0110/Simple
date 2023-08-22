#include "MoveGenerator.h"

#include <cassert>

#include "Attacks.h"

using namespace SimpleChessEngine;

template <bool only_attacks>
MoveGenerator::Moves MoveGenerator::GenerateMoves(Position& position) const
{
  // create moves, max amount moves 218
  Moves moves;
  static constexpr size_t kMaxMovesAmount = 218;
  moves.reserve(kMaxMovesAmount);

  // get all pieces
  auto pieces = position.GetPieces(position.GetSideToMove());

  // generate moves for each piece
  while (const auto from = pieces.GetFirstBit())
  {
    // get first piece
    pieces.Reset(*from);

    // generate moves for piece
    auto moves_for_piece = GenerateMovesForPiece(position, *from);

    // add moves
    moves.insert(moves.end(), moves_for_piece.begin(), moves_for_piece.end());
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
  static constexpr size_t kMaxMovesAmount = 218;
  moves.reserve(kMaxMovesAmount);

  // get piece and side to move
  const auto piece = position.GetPiece(from);

  // check if piece exists
  assert(position.GetAllPieces().test(from));

  switch (piece)
  {
    case Piece::kPawn:
      moves = GenerateMovesFromSquare<Piece::kPawn>(position, from);
      break;
    case Piece::kKnight:
      // generate knight moves
      moves = GenerateMovesFromSquare<Piece::kKnight>(position, from);
      break;
    case Piece::kBishop:
      // generate bishop moves
      moves = GenerateMovesFromSquare<Piece::kBishop>(position, from);
      break;
    case Piece::kRook:
      // generate rook moves
      moves = GenerateMovesFromSquare<Piece::kRook>(position, from);
      break;
    case Piece::kQueen:
      // generate queen moves
      moves = GenerateMovesFromSquare<Piece::kQueen>(position, from);
      break;
    case Piece::kKing:
      // generate king moves
      moves = GenerateMovesFromSquare<Piece::kKing>(position, from);
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
  static constexpr size_t kMaxMoves = 27;
  moves.reserve(kMaxMoves);

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

    if (auto move = Move{from, to.value(), position.GetPiece(to.value())};
        IsMoveValid(position, move))
    {
      moves.emplace_back(move);
    }
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

    return ApplyPromotions(std::move(moves), position, from);
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

  moves.insert(moves.end(), ordinary_moves.begin(), ordinary_moves.end());

  return ApplyPromotions(std::move(moves), position, from);
}

MoveGenerator::Moves MoveGenerator::GenerateAttacksForPawn(Position& position,
                                                           const BitIndex from)
{
  Moves moves;
  static constexpr size_t kMaxMoves = 2;
  moves.reserve(kMaxMoves);

  auto side_to_move = position.GetSideToMove();

  for (static constexpr std::array<std::array<Compass, 2>, kColors>
           kPawnAttackDirections = {{{Compass::kNorthWest, Compass::kNorthEast},
                                     {Compass::kSouthWest,
                                      Compass::kSouthEast}}};
       auto shift : kPawnAttackDirections[static_cast<size_t>(side_to_move)])
  {
    if (const BitIndex to = from + static_cast<int>(shift);
        IsShiftValid(to, from))
    {
      Move move{from, to, position.GetPiece(to)};

      if (position.GetPieces(Flip(side_to_move)).Test(to))
      {
        if (IsMoveValid(position, move)) moves.emplace_back(move);
      }
      else if (position.GetEnPassantSquare() == to)
      {
        move.captured_piece = Piece::kPawn;
        if (IsMoveValid(position, move)) moves.emplace_back(move);
      }
    }
  }

  return moves;
}

MoveGenerator::Moves MoveGenerator::GenerateMovesForPawn(Position& position,
                                                         const BitIndex from)
{
  Moves moves;
  static constexpr size_t kMaxMoves = 2;
  static constexpr std::array kPawnDoublePushRank = {1, 6};
  moves.reserve(kMaxMoves);

  auto [file, rank] = GetCoordinates(from);
  auto side_to_move = position.GetSideToMove();

  if (const auto to =
          from + static_cast<int>(
                     kPawnMoveDirection[static_cast<size_t>(side_to_move)]);
      !position.GetPiece(to))
  {
    if (auto move = Move{from, to}; IsMoveValid(position, move))
    {
      moves.emplace_back(move);
    }
  }

  if (rank == kPawnDoublePushRank[static_cast<size_t>(side_to_move)] &&
      (position.GetAllPieces() &
       kDoubleMoveSpan[static_cast<size_t>(side_to_move)][file])
          .None())
  {
    const auto to =
        from + 2 * static_cast<int>(
                       kPawnMoveDirection[static_cast<size_t>(side_to_move)]);
    if (auto move = Move{from, to}; IsMoveValid(position, move))
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
  auto side_to_move = position.GetSideToMove();

  if (static constexpr std::array kIsPromoting = {6, 1};
      GetCoordinates(from).second ==
      kIsPromoting[static_cast<size_t>(side_to_move)])
  {
    static constexpr std::array kPiecesToPromoteTo = {
        Piece::kKnight, Piece::kBishop, Piece::kRook, Piece::kQueen};

    moves.reserve(moves.size() * kPiecesToPromoteTo.size());

    const auto end = moves.end();

    for (auto it = moves.begin(); it != end; ++it)
    {
      (*it).promoted_to = kPiecesToPromoteTo.front();

      for (size_t piece = 1; piece < kPiecesToPromoteTo.size(); piece++)
      {
        moves.emplace_back(*it);
        moves.back().promoted_to = kPiecesToPromoteTo[piece];
      }
    }
  }

  return moves;
}