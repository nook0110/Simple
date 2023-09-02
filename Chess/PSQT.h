#pragma once

#include "Evaluation.h"

namespace SimpleChessEngine
{
  inline std::array<std::array<std::array<TaperedEval, kBoardArea>, kPieceTypes>, kColors> kPSQT =
  {
    {
      // White pieces table
      {
        {
          // Empty squares values are irrelevant since kNone is not being placed or removed at any time
          {},
          // Pawns
          {
            {
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {}
            }
          },
          // Knights
          {
            {
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {}
            }
          },
          // Bishops
          {
            {
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {}
            }
          },
          // Rooks
          {
            {
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {}
            }
          },
          // Queen
          {
            {
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {}
            }
          },
          // King
          {
            {
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {},
              {}, {}, {}, {}, {}, {}, {}, {}
            }
          }
        }
      }
      // Black pieces table is initialized symmetrically to the white pieces table
    }
  };

  inline void InitPSQT()
  {
    for (auto piece : { Piece::kPawn, Piece::kKnight, Piece::kBishop, Piece::kRook, Piece::kQueen, Piece::kKing })
    {
      for (BitIndex square = 0; square < kBoardArea; ++square)
      {
        const auto [file, rank] = GetCoordinates(square);
        const BitIndex new_square = GetSquare(file, 7 - rank);
        kPSQT[static_cast<size_t>(Player::kBlack)][static_cast<size_t>(piece)][new_square] =
          kPSQT[static_cast<size_t>(Player::kWhite)][static_cast<size_t>(piece)][square];
      }
    }
  }
}