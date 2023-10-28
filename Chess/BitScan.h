#pragma once
#include <cassert>

#ifdef __GNUC__
#define USE_GCC_BUILTINS
#pragma GCC target("avx2,bmi,bmi2,popcnt,lzcnt")
#elif defined(_MSC_VER)
#define USE_MSVC_INTRINSICS
#pragma intrinsic(_BitScanForward)
#endif

using BitIndex = unsigned long;

inline BitIndex BitScan(const size_t mask)
{
  assert(mask);
#ifdef USE_GCC_BUILTINS
  return __builtin_ctzll(mask);
#elif defined(USE_MSVC_INTRINSICS)
  unsigned long index;
  _BitScanForward64(&index, mask);
  return index;
#endif
}

#undef USE_GCC_BUILTINS
#undef USE_MSVC_INTRINSICS
