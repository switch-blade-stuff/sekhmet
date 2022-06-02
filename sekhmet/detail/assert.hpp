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
 * Created by switchblade on 2021-10-25
 */

#pragma once

#include <cstdio>
#include <cstdlib>

#include "define.h"

namespace sek::detail
{
	[[noreturn]] [[maybe_unused]] inline void assert_never_reached_impl(const char *file, std::size_t line, const char *func)
	{
		fprintf(stderr, "Reached unreachable code at '%s:%lu' in '%s'. This is an internal error.\n", file, line, func);
		std::abort();
	}
	[[maybe_unused]] inline void
		assert_impl(bool cnd, const char *cnd_str, const char *file, std::size_t line, const char *func, const char *msg)
	{
		if (!cnd) [[unlikely]]
		{
			fprintf(stderr, "Assertion ");
			if (cnd_str) [[likely]]
				fprintf(stderr, "(%s) ", cnd_str);

			fprintf(stderr, "failed at '%s:%lu' in '%s'", file, line, func);
			if (msg) [[likely]]
				fprintf(stderr, ": %s", msg);
			fputc('\n', stderr);

			std::abort();
		}
	}
}	 // namespace sek::detail

#define SEK_ASSERT_2(cnd, msg) sek::detail::assert_impl((cnd), (#cnd), (SEK_FILE), (SEK_LINE), (SEK_PRETTY_FUNC), (msg))
#define SEK_ASSERT_1(cnd) SEK_ASSERT_2(cnd, nullptr)

/** Same as regular SEK_ASSERT, except applies even when SEK_NO_DEBUG_ASSERT is defined. */
#define SEK_ASSERT_ALWAYS(...) SEK_GET_MACRO_2(__VA_ARGS__, SEK_ASSERT_2, SEK_ASSERT_1)(__VA_ARGS__)

/** Asserts that the code should never be reached. */
#define SEK_NEVER_REACHED sek::detail::assert_never_reached_impl((SEK_FILE), (SEK_LINE), (SEK_PRETTY_FUNC))

#if !defined(SEK_NO_DEBUG_ASSERT) && !defined(NDEBUG)
/** Assert that supports an optional message, prints the enclosing function name and terminates using exit(1).
 * @note Currently the message can only be a char literal. */
#define SEK_ASSERT(...) SEK_ASSERT_ALWAYS(__VA_ARGS__)
#else
#define SEK_ASSERT(...)
#endif

#define SEK_ASSERT_NORETURN_2(eval, msg)                                                                               \
	do {                                                                                                               \
		eval;                                                                                                          \
		if (msg) fprintf(stderr, "%s\n", msg);                                                                         \
		abort();                                                                                                       \
	} while (0);
#define SEK_ASSERT_NORETURN_1(eval) SEK_ASSERT_NORETURN_2(eval, nullptr)

/** Asserts that an expression must not return. */
#define SEK_ASSERT_NORETURN(...)                                                                                       \
	SEK_GET_MACRO_2(__VA_ARGS__, SEK_ASSERT_NORETURN_2, SEK_ASSERT_NORETURN_1)                                         \
	(__VA_ARGS__)
