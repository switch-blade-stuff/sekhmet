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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2021-11-06
 */

#pragma once

/* CPU architecture index_selector.
 * Credit: FreakAnon - https://stackoverflow.com/a/66249936/13529335 */
#if defined(__x86_64__) || defined(_M_X64)
#define SEK_ARCH_x86_64
#define SEK_ARCH_x86
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define SEK_ARCH_x86_32
#define SEK_ARCH_x86
#elif defined(__ARM_ARCH_2__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM2
#elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM3
#elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM4T
#elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM5
#elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM6T2
#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) ||   \
	defined(__ARM_ARCH_6ZK__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM6
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) ||   \
	defined(__ARM_ARCH_7S__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM7
#elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM7A
#elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM7R
#elif defined(__ARM_ARCH_7M__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM7M
#elif defined(__ARM_ARCH_7S__)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM7S
#elif defined(__aarch64__) || defined(_M_ARM64)
#define SEK_ARCH_ARM
#define SEK_ARCH_ARM64
#elif defined(mips) || defined(__mips__) || defined(__mips)
#define SEK_ARCH_MIPS
#elif defined(__sh__)
#define SEK_ARCH_SUPERH
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) ||                  \
	defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
#define SEK_ARCH_POWERPC
#elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
#define SEK_ARCH_POWERPC
#define SEK_ARCH_POWERPC64
#elif defined(__sparc__) || defined(__sparc)
#define SEK_ARCH_SPARC
#elif defined(__m68k__)
#define SEK_ARCH_M68K
#else
#define SEK_ARCH_UNKNOWN
#endif

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) ||                                             \
	(defined(__BYTE_ORDER__) && __BYTE_ORDER == __BIG_ENDIAN) || defined(SEK_ARCH_POWERPC)
#define SEK_ARCH_BIG_ENDIAN
#else
#define SEK_ARCH_LITTLE_ENDIAN
#endif