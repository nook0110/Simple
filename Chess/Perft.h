#pragma once
#include <ostream>

#include "MoveGenerator.h"
#include "Position.h"
namespace SimpleChessEngine
{
template <bool print = true>
size_t Perft(std::ostream& o_stream, Position& position, const size_t depth)
{
  if (depth == 0) return 1;

  const auto moves =
      MoveGenerator{}.GenerateMoves<MoveGenerator::Type::kDefault>(position);

  size_t answer{};

  for (const auto& move : moves)
  {
    if constexpr (print)
    {
      std::visit([&o_stream](const auto& unwrapped_move)
                 { o_stream << unwrapped_move; },
                 move);
      o_stream << ": ";
    }

    size_t cur_answer{};

    if (depth > 1)
    {
      const auto irreversible_data = position.GetIrreversibleData();
      position.DoMove(move);

      cur_answer = Perft<false>(o_stream, position, depth - 1);
      if constexpr (print)
      {
        o_stream << cur_answer << std::endl;
      }
      position.UndoMove(move, irreversible_data);
    }
    else
    {
      cur_answer = Perft<false>(o_stream, position, depth - 1);
      if constexpr (print)
      {
        o_stream << cur_answer << std::endl;
      }
    }

    answer += cur_answer;
  }
  if constexpr (print)
  {
    o_stream << "Leafs: " << answer << std::endl;
  }
  return answer;
}
}  // namespace SimpleChessEngine