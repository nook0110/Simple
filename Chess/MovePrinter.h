#pragma once
#include <ostream>

#include "Move.h"
#include "Utility.h"

namespace SimpleChessEngine
{
inline std::ostream& PrintFile(const File file,
                               std::ostream& stream = std::cout)
{
  stream << file + 'a';
  return stream;
}

inline std::ostream& PrintRank(const Rank rank,
                               std::ostream& stream = std::cout)
{
  stream << rank;
  return stream;
}

inline std::ostream& PrintCoordinates(const Coordinates coordinates,
                                      std::ostream& stream)
{
  PrintRank(coordinates.first, stream);
  PrintFile(coordinates.second, stream);

  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const DefaultMove move)
{
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
inline std::ostream& operator<<(std::ostream& stream, const DoublePush move)
{
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
inline std::ostream& operator<<(std::ostream& stream, const EnCroissant move)
{
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}
inline std::ostream& operator<<(std::ostream& stream, const Promotion move)
{
  const auto from = GetCoordinates(move.from);
  const auto to = GetCoordinates(move.to);

  PrintCoordinates(from, stream);
  PrintCoordinates(to, stream);

  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, Castling move)
{
  static constexpr std::array kCastlingNames = {std::string_view{"O-O"},
                                                std::string_view{"O-O"}};

  return stream << kCastlingNames[static_cast<size_t>(move.side)];
}
}  // namespace SimpleChessEngine