/*
 * Created by switchblade on 23/09/22
 */

#pragma once

#ifdef __cplusplus

#include <cstddef>
#include <cstdint>

#else

#include <stddef.h>
#include <stdint.h>

#endif

#include "detail/arch.h"
#include "detail/platform.h"

#define SEK_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define SEK_GET_MACRO_1(_1, MACRO, ...) MACRO
#define SEK_GET_MACRO_2(_1, _2, MACRO, ...) MACRO
#define SEK_GET_MACRO_3(_1, _2, _3, MACRO, ...) MACRO
#define SEK_GET_MACRO_4(_1, _2, _3, _4, MACRO, ...) MACRO
#define SEK_GET_MACRO_5(_1, _2, _3, _4, _5, MACRO, ...) MACRO

#define SEK_CONCAT_2(a, b) a##b
#define SEK_CONCAT_3(a, b, c) a##b##c
#define SEK_CONCAT_4(a, b, c, d) a##b##c##d
#define SEK_CONCAT_5(a, b, c, d, e) a##b##c##d##e
#define SEK_CONCAT_6(a, b, c, d, e, f) a##b##c##d##e##f
#define SEK_CONCAT(a, ...)                                                                                             \
	SEK_GET_MACRO_5(__VA_ARGS__, SEK_CONCAT_6, SEK_CONCAT_5, SEK_CONCAT_4, SEK_CONCAT_3, SEK_CONCAT_2)                 \
	(a, __VA_ARGS__)

#define SEK_KB(val) (1024 * (val))
#define SEK_MB(val) (1024 * SEK_KB(val))
#define SEK_GB(val) (1024 * SEK_MB(val))
#define SEK_TB(val) (1024 * SEK_GB(val))

#if !defined(SEK_DEBUG) && !defined(NDEBUG)
#define SEK_DEBUG
#endif

#ifdef SEK_EDITOR
#define SEK_IF_EDITOR(x) x
#else
#define SEK_IF_EDITOR(...)
#endif
