/*
 * Created by switchblade on 2021-10-20
 */

#pragma once

#include <cstring>

#include "../define.h"
#include "meta_util.hpp"

namespace sek::detail
{
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length_slow(I str) noexcept
	{
		for (std::size_t i = 0;; ++i)
			if (*str++ == '\0') return i;
	}
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length_slow(I str, std::size_t max) noexcept
	{
		for (std::size_t i = 0;; ++i)
			if (i == max || *str++ == '\0') return i;
	}
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		return str_length_slow(str);
	}
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		return str_length_slow(str, max);
	}
	template<forward_iterator_for<char> I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str);
		else
			return static_cast<std::size_t>(strlen(std::to_address(str)));
	}
	template<forward_iterator_for<char> I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str, max);
		else
			return static_cast<std::size_t>(strnlen(std::to_address(str), static_cast<std::size_t>(max)));
	}
	template<forward_iterator_for<char8_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str);
		else
		{
			auto ptr = reinterpret_cast<const char *>(std::to_address(str));
			return static_cast<std::size_t>(strlen(ptr));
		}
	}
	template<forward_iterator_for<char8_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str, max);
		else
		{
			auto ptr = reinterpret_cast<const char *>(std::to_address(str));
			return static_cast<std::size_t>(strnlen(ptr, static_cast<std::size_t>(max)));
		}
	}
	template<forward_iterator_for<wchar_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str);
		else
			return static_cast<std::size_t>(wcslen(std::to_address(str)));
	}
	template<forward_iterator_for<wchar_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str, max);
		else
			return static_cast<std::size_t>(wcsnlen(std::to_address(str), static_cast<std::size_t>(max)));
	}

	template<std::forward_iterator Il, std::forward_iterator Ir>
	[[nodiscard]] constexpr auto str_compare(Il lhs_begin, Il lhs_end, Ir rhs_begin, Ir rhs_end) noexcept
	{
		return std::lexicographical_compare_three_way(lhs_begin, lhs_end, rhs_begin, rhs_end);
	}
	template<std::forward_iterator Il, std::forward_iterator Ir>
	[[nodiscard]] constexpr auto str_equal(Il lhs_begin, Il lhs_end, Ir rhs_begin, Ir rhs_end) noexcept
	{
		return std::equal(lhs_begin, lhs_end, rhs_begin, rhs_end);
	}

	template<auto bad_idx, std::forward_iterator HaystackIterator, std::forward_iterator NeedleIterator>
	[[nodiscard]] constexpr decltype(bad_idx) find_first(HaystackIterator haystack_begin,
														 HaystackIterator haystack_end,
														 NeedleIterator needle_first,
														 NeedleIterator needle_last) noexcept
	{
		auto haystack_bottom = haystack_begin, haystack_pos = haystack_begin;
		auto needle = needle_first;
		while (haystack_pos != haystack_end)
		{
			if (*haystack_pos++ != *needle++)
			{
				haystack_pos = ++haystack_bottom;
				needle = needle_first;
			}
			else if (needle == needle_last)
				return static_cast<decltype(bad_idx)>(std::distance(haystack_begin, haystack_bottom));
		}
		return bad_idx;
	}
	template<auto bad_idx, std::bidirectional_iterator HaystackIterator, std::bidirectional_iterator NeedleIterator>
	[[nodiscard]] constexpr decltype(bad_idx) find_last(HaystackIterator haystack_begin,
														HaystackIterator haystack_end,
														NeedleIterator needle_first,
														NeedleIterator needle_last) noexcept
	{
		auto haystack_top = haystack_end, haystack = haystack_end;
		auto needle = needle_last;
		for (;;)
		{
			if (needle == needle_first)
				return static_cast<decltype(bad_idx)>(std::distance(haystack_begin, haystack));
			else if (haystack_begin == haystack) [[unlikely]]
				return bad_idx;
			else if (*(--haystack) != *(--needle))
			{
				haystack = --haystack_top;
				needle = needle_last;
			}
		}
	}

	template<auto bad_idx, std::forward_iterator HaystackIterator, std::forward_iterator NeedleIterator>
	[[nodiscard]] constexpr decltype(bad_idx) find_first_of(HaystackIterator haystack_begin,
															HaystackIterator haystack_end,
															NeedleIterator needle_first,
															NeedleIterator needle_last) noexcept
	{
		for (auto haystack = haystack_begin; haystack != haystack_end; ++haystack)
			for (auto needle = needle_first; needle != needle_last; ++needle)
			{
				if (*haystack == *needle)
					return static_cast<decltype(bad_idx)>(std::distance(haystack_begin, haystack));
			}
		return bad_idx;
	}
	template<auto bad_idx, std::bidirectional_iterator HaystackIterator, std::forward_iterator NeedleIterator>
	[[nodiscard]] constexpr decltype(bad_idx) find_last_of(HaystackIterator haystack_begin,
														   HaystackIterator haystack_end,
														   NeedleIterator needle_first,
														   NeedleIterator needle_last) noexcept
	{
		for (auto haystack = haystack_end; haystack-- != haystack_end;)
			for (auto needle = needle_first; needle != needle_last; ++needle)
			{
				if (*needle == *haystack)
					return static_cast<decltype(bad_idx)>(std::distance(haystack_begin, haystack));
			}
		return bad_idx;
	}

	template<auto bad_idx, std::forward_iterator HaystackIterator, std::forward_iterator NeedleIterator>
	[[nodiscard]] constexpr decltype(bad_idx) find_first_not_of(HaystackIterator haystack_begin,
																HaystackIterator haystack_end,
																NeedleIterator needle_first,
																NeedleIterator needle_last) noexcept
	{
		for (auto haystack = haystack_begin; haystack != haystack_end; ++haystack)
			for (auto needle = needle_first;; ++needle)
			{
				if (needle == needle_last)
					return static_cast<decltype(bad_idx)>(std::distance(haystack_begin, haystack));
				else if (*needle == *haystack)
					break;
			}
		return bad_idx;
	}
	template<auto bad_idx, std::bidirectional_iterator HaystackIterator, std::forward_iterator NeedleIterator>
	[[nodiscard]] constexpr decltype(bad_idx) find_last_not_of(HaystackIterator haystack_begin,
															   HaystackIterator haystack_end,
															   NeedleIterator needle_first,
															   NeedleIterator needle_last) noexcept
	{
		for (; haystack_begin != haystack_end--;)
		{
			for (auto needle = needle_first;; ++needle)
			{
				if (needle == needle_last)
					return static_cast<decltype(bad_idx)>(std::distance(haystack_begin, haystack_end));
				else if (*needle == *haystack_end)
					break;
			}
		}
		return bad_idx;
	}

	template<std::forward_iterator HaystackIterator, std::forward_iterator NeedleIterator>
	[[nodiscard]] constexpr bool has_prefix(HaystackIterator haystack_begin,
											HaystackIterator haystack_end,
											NeedleIterator needle_first,
											NeedleIterator needle_last) noexcept
	{
		for (;;)
		{
			if (needle_first == needle_last)
				return true;
			else if (haystack_begin == haystack_end || *needle_first++ != *haystack_begin++)
				return false;
		}
	}
	template<std::bidirectional_iterator HaystackIterator, std::forward_iterator NeedleIterator>
	[[nodiscard]] constexpr bool has_postfix(HaystackIterator haystack_begin,
											 HaystackIterator haystack_end,
											 NeedleIterator needle_first,
											 NeedleIterator needle_last) noexcept
	{
		for (;;)
		{
			if (needle_first == needle_last)
				return true;
			else if (haystack_begin == haystack_end || *(--needle_last) != *(--haystack_end))
				return false;
		}
	}
}	 // namespace sek::detail