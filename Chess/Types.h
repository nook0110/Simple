#pragma once

#ifdef __SIZEOF_INT128__
constexpr bool kHasUInt128Support = true;
using uint128_t = __uint128_t;
static_assert(sizeof(uint128_t) == 16);
#else
constexpr bool kHasUInt128Support = false;
#warning "uint128_t is not supported"
#endif