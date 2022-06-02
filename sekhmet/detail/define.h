/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2021-10-14
 */

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

#define SEK_GET_MACRO_1(_1, MACRO, ...) MACRO
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

#ifdef __cplusplus
#define SEK_HAS_OVERLOAD(func, ...) requires { func(__VA_ARGS__); }
#define SEK_REQUIRES_OVERLOAD(func, ...) requires(SEK_HAS_OVERLOAD(func, __VA_ARGS__))
#endif