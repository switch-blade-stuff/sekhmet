//
// Created by switchblade on 07/05/22.
//

#pragma once

namespace sek
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
}