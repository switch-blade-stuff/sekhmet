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
 * Created by switchblade on 07/05/22
 */

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