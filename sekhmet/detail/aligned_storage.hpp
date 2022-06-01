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
 * Created by switchblade on 2021-12-03
 */

#pragma once

#include <bit>
#include <cstddef>

namespace sek
{
	template<std::size_t Size, std::size_t Align>
	class alignas(Align) aligned_storage
	{
	public:
		template<typename T>
		requires(sizeof(T) <= Size) [[nodiscard]] constexpr T *get() noexcept { return std::bit_cast<T *>(data()); }
		template<typename T>
		requires(sizeof(T) <= Size) [[nodiscard]] constexpr const T *get() const noexcept
		{
			return std::bit_cast<const T *>(data());
		}

		[[nodiscard]] constexpr void *data() noexcept { return static_cast<void *>(data_bytes); }
		[[nodiscard]] constexpr const void *data() const noexcept { return static_cast<const void *>(data_bytes); }

	private:
		std::byte data_bytes[Size];
	};

	template<typename T>
	using type_storage = aligned_storage<sizeof(T), alignof(T)>;
}	 // namespace sek