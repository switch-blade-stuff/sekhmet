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
 * Created by switchblade on 30/04/22
 */

#pragma once

#include "util.hpp"

namespace sek::math
{
	enum class storage_policy : int
	{
		/** @brief Values are stored with potential over-alignment to allow for SIMD optimizations. */
		OPTIMAL,
		/** @brief Values are tightly packed in memory.
		 * @note Packed storage is not SIMD-optimized. */
		PACKED
	};

	namespace detail
	{
		template<typename, std::size_t>
		struct simd_t;

		template<typename T, std::size_t N, typename = void>
		struct simd_defined : std::false_type
		{
		};
		template<typename T, std::size_t N>
		struct simd_defined<T, N, std::void_t<decltype(sizeof(simd_t<T, N>))>> : std::true_type
		{
		};
		template<typename T, std::size_t N>
		concept simd_exists = (simd_defined<T, N>::value && !std::is_empty_v<simd_t<T, N>>);
	}	 // namespace detail
}	 // namespace sek::math
