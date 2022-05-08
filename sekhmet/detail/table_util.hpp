//
// Created by switchblade on 07/05/22.
//

#pragma once

#include <utility>

namespace sek
{
	/** @brief Forwards the passed argument. */
	struct forward_identity
	{
		template<typename U>
		constexpr decltype(auto) operator()(U &&val) const noexcept
		{
			return std::forward<U>(val);
		}
	};

	/** @brief Returns first element of a pair (`value.first`). */
	struct pair_first
	{
		template<typename T>
		constexpr auto &operator()(T &value) const noexcept
		{
			return value.first;
		}
	};
}