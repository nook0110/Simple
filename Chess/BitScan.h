#pragma once
#include <intrin.h>

#include <bitset>

#pragma intrinsic(_BitScanForward)

using BitIndex = size_t;

inline std::optional<BitIndex> BitScan(const size_t mask)
{
  unsigned long index;
  if (_BitScanForward64(&index, mask))
  {
    return index;
  }

  return std::nullopt;
}

inline std::optional<BitIndex> BitScan(const std::bitset<64>& mask)
{
  unsigned long index;
  if (_BitScanForward64(&index, mask.to_ullong()))
  {
    return index;
  }

  return std::nullopt;
}
