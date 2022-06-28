/*
 * Created by switchblade on 26/06/22
 */

#pragma once

#include "sekhmet/detail/define.h"

#ifndef SEK_NO_SIMD

#ifdef __SSE__
#define SEK_USE_SSE
#ifdef __SSE2__
#define SEK_USE_SSE2
#ifdef __SSE3__
#define SEK_USE_SSE3
#ifdef __SSSE3__
#define SEK_USE_SSSE3
#ifdef __SSE4_1__
#define SEK_USE_SSE4
#define SEK_USE_SSE4_1
#ifdef __SSE4_2__
#define SEK_USE_SSE4_2
#endif /* __SSE4_2__ */
#endif /* __SSE4_1__ */
#endif /* __SSSE3__ */
#endif /* __SSE3__ */
#endif /* __SSE2__ */
#endif /* __SSE__ */

#ifdef __FMA__
#define SEK_USE_FMA
#endif /* __FMA__ */

/* TODO: Implement AVX support */
// #ifdef __AVX__
// #define SEK_USE_AVX
// #ifdef __AVX2__
// #undef SEK_USE_AVX2
// #endif /* __AVX2__ */
// #endif /* __AVX__ */

#endif