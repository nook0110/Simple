#pragma once
#include <intrin.h>

#include <bitset>

#pragma intrinsic(_BitScanForward)

using BitIndex = unsigned long;

inline BitIndex BitScan(const size_t mask)
{
  BitIndex index;
  if (_BitScanForward64(&index, mask))
  {
    return index;
  }

  return -1;
}

inline BitIndex BitScan(const std::bitset<64>& mask)
{
  BitIndex index;
  if (_BitScanForward64(&index, mask.to_ullong()))
  {
    return index;
  }

  return -1;
}
