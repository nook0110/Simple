#pragma once

#ifdef __GNUC__
#define USE_GCC_BUILTINS
#pragma GCC target("avx2,bmi,bmi2,popcnt,lzcnt")
#elif defined(_MSC_VER)
#define USE_MSVC_INTRINSICS
#include <intrin.h>

#include "nmmintrin.h"
#pragma intrinsic(_BitScanForward)
#endif

#include <bitset>

using BitIndex = size_t;

inline std::optional<BitIndex> BitScan(const size_t mask)
{
  unsigned long index;
#ifdef USE_GCC_BUILTINS
  const size_t idx = __builtin_ctzll(mask);
  return idx == 64 ? std::nullopt : idx;
#elif defined(USE_MSVC_INTRINSICS)
  if (_BitScanForward64(&index, mask))
  {
    return index;
  }
  return std::nullopt;
#endif
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

#undef USE_GCC_BUILTINS
#undef USE_MSVC_INTRINSICS
