//
// Created by switchblade on 2022-03-22.
//

#pragma once

#include <algorithm>
#include <concepts>

#include "define.h"
#include <type_traits>

namespace sek::detail
{
	/** @brief Returns first element of a pair (`value.first`). */
	struct pair_first
	{
		template<typename T>
		constexpr auto &operator()(T &value) const noexcept
		{
			return value.first;
		}
	};

	/** Relocates object F from one location to another. */
	template<typename T>
	void relocate(T *from, T *to)
	{
		if constexpr (std::is_trivially_copyable_v<T>)
			*to = *from;
		else
		{
			std::construct_at(to, std::move(*from));
			std::destroy_at(from);
		}
	}
	/** Relocates elements from source iterator range to target iterator range.
	 * First relocation is first -> to.
	 * @param first Iterator to the start of the source range.
	 * @param last Iterator to the end of the source range.
	 * @param to Iterator to the start of the target range.
	 * @return Iterator to the end of the target range. */
	template<std::forward_iterator IteratorFrom, std::forward_iterator IteratorTo>
	constexpr IteratorTo relocate_all(IteratorFrom first, IteratorFrom last, IteratorTo to)
	{
		if constexpr (std::contiguous_iterator<IteratorFrom> && std::contiguous_iterator<IteratorTo> &&
					  std::is_trivially_copyable_v<std::iter_value_t<IteratorFrom>>)
			return std::copy(first, last, to);
		else
		{
			for (; first != last; ++first, ++to) relocate(std::to_address(first), std::to_address(to));
			return to;
		}
	}
	/** Relocates elements from source iterator range to target iterator range.
	 * First relocation is std::prev(last) -> std::prev(to_end).
	 * @param first Iterator to the start of the source range.
	 * @param last Iterator to the end of the source range.
	 * @param to_end Iterator to the end of the target range.
	 * @return Iterator to the start of the target range. */
	template<std::bidirectional_iterator IteratorFrom, std::bidirectional_iterator IteratorTo>
	constexpr IteratorTo relocate_all_reverse(IteratorFrom first, IteratorFrom last, IteratorTo to_end)
	{
		if constexpr (std::contiguous_iterator<IteratorFrom> && std::contiguous_iterator<IteratorTo> &&
					  std::is_trivially_copyable_v<std::iter_value_t<IteratorFrom>>)
			return std::move_backward(first, last, to_end);
		else
		{
			while (first != last) relocate(std::to_address(--last), std::to_address(--to_end));
			return to_end;
		}
	}

}	 // namespace sek::detail