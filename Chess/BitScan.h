#pragma once

#define USE_MSVC_INTRINSICS

#pragma intrinsic(_BitScanForward)

using BitIndex = unsigned char;

inline std::optional<BitIndex> BitScan(const size_t mask)
{
  unsigned long index;

  if (_BitScanForward64(&index, mask))
  {
    return index;
  }
  return std::nullopt;
}
