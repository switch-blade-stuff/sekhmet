//
// Created by switchblade on 2021-12-16.
//

#pragma once

#include "sekhmet/detail/define.h"

#ifdef SEK_ARCH_x86

#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>

#ifdef SEK_NO_SIMD
#ifdef SEK_USE_SSE
#undef SEK_USE_SSE
#endif
#ifdef SEK_USE_AVX
#undef SEK_USE_AVX
#endif
#endif

#if defined(SEK_USE_SSE) && (!defined(__SSE__))
#undef SEK_USE_SSE
#endif
#if defined(SEK_USE_SSE2) && (!defined(__SSE2__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSE2
#endif
#if defined(SEK_USE_SSE3) && (!defined(__SSE3__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSE3
#endif
#if defined(SEK_USE_SSSE3) && (!defined(__SSSE3__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSSE3
#endif
#if defined(SEK_USE_SSE4) && !defined(SEK_USE_SSE)
#undef SEK_USE_SSE4
#endif
#if defined(SEK_USE_SSE4_1) && (!defined(__SSE4_1__) || !defined(SEK_USE_SSE4))
#undef SEK_USE_SSE4_1
#endif
#if defined(SEK_USE_SSE4_2) && (!defined(__SSE4_2__) || !defined(SEK_USE_SSE4))
#undef SEK_USE_SSE4_2
#endif

#if defined(SEK_USE_AVX) && !defined(__AVX__)
#undef SEK_USE_AVX
#endif
#if defined(SEK_USE_AVX2) && (!defined(__AVX2__) || !defined(SEK_USE_AVX))
#undef SEK_USE_AVX2
#endif

#endif