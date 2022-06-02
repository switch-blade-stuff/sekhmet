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
 * Created by switchblade on 31/05/22
 */

#pragma once

#include <exception>
#include <span>
#include <string>
#include <vector>

#include "define.h"

namespace sek
{
	/** @brief Base type for exceptions used to concatenate multiple exceptions together
	 * (ex. to return exceptions from worker threads).
	 * @note Requires dynamic memory allocation for internal array. */
	class compound_exception
	{
	public:
		constexpr compound_exception() noexcept = default;

		/** Initializes compound exception from an initializer list of exception pointers. */
		constexpr compound_exception(std::initializer_list<std::exception_ptr> il) : exceptions(il) {}

		/** Pushes current exception the internal storage. Equivalent to `push(std::current_exception())`. */
		void push_current() { push(std::current_exception()); }
		/** Pushes exception pointer to the internal storage. */
		SEK_API void push(std::exception_ptr ptr);

		/** Returns a span of nested exceptions. */
		[[nodiscard]] constexpr std::span<const std::exception_ptr> nested() const noexcept
		{
			return std::span<const std::exception_ptr>{exceptions.data(), exceptions.size()};
		}
		/** Returns a string consisting of nested exceptions' messages. */
		[[nodiscard]] SEK_API std::string message() const;

		constexpr void swap(compound_exception &other) noexcept { exceptions.swap(other.exceptions); }
		friend constexpr void swap(compound_exception &a, compound_exception &b) noexcept { a.swap(b); }

	private:
		std::vector<std::exception_ptr> exceptions;
	};
}	 // namespace sek