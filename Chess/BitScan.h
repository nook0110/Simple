#pragma once
#include <intrin.h>

#include <bitset>

#pragma intrinsic(_BitScanForward)

using BitIndex = int;

inline BitIndex BitScan(const size_t mask)
{
  unsigned long index;
  if (_BitScanForward64(&index, mask))
  {
    return index;
  }

  return -1;
}

inline BitIndex BitScan(const std::bitset<64>& mask)
{
  unsigned long index;
  if (_BitScanForward64(&index, mask.to_ullong()))
  {
    return index;
  }

  return -1;
}
