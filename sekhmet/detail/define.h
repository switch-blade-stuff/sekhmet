//
// Created by switchblade on 2021-10-14.
//

#pragma once

#ifdef __cplusplus

#include <cstddef>
#include <cstdint>

#else

#include <stddef.h>
#include <stdint.h>

#endif

#include "arch.h"
#include "platform.h"

#ifdef __STDC_NO_ATOMICS__
#define SEK_NO_ATOMICS
#endif

#define SEK_THREAD_OPTIONAL(content) content
#define SEK_THREAD_LOCAL thread_local

#define SEK_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define SEK_CONCAT(a, b) a##b

#define SEK_GET_MACRO_2(_1, _2, MACRO, ...) MACRO
#define SEK_GET_MACRO_3(_1, _2, _3, MACRO, ...) MACRO
#define SEK_GET_MACRO_4(_1, _2, _3, _4, MACRO, ...) MACRO
#define SEK_GET_MACRO_5(_1, _2, _3, _4, _5, MACRO, ...) MACRO

#define SEK_KB(val) (1024 * (val))
#define SEK_MB(val) (1024 * SEK_KB(val))
#define SEK_GB(val) (1024 * SEK_MB(val))
#define SEK_TB(val) (1024 * SEK_GB(val))

#ifdef SEK_EDITOR
#define SEK_IF_EDITOR(x) x
#else
#define SEK_IF_EDITOR(...)
#endif