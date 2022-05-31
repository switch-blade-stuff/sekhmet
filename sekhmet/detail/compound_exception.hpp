//
// Created by switchblade on 31/05/22.
//

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