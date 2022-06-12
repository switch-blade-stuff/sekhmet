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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 09/06/22
 */

#pragma once

#include <utility>

namespace sek::system::detail
{
	class native_file_handle
	{
	public:
		typedef int native_handle_type;

	public:
		constexpr native_file_handle() noexcept = default;

		SEK_API bool open(const char *path, openmode mode) noexcept;
		SEK_API bool close() noexcept;

		SEK_API std::int64_t read(void *dst, std::size_t n) const noexcept;
		SEK_API std::int64_t write(const void *src, std::size_t n) const noexcept;
		SEK_API std::int64_t seek(std::int64_t off, seek_dir way) const noexcept;
		inline std::int64_t tell() const noexcept { return this->seek(0, cur); }
		SEK_API bool sync() const noexcept;

		[[nodiscard]] constexpr bool is_open() const noexcept { return m_descriptor >= 0; }
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_descriptor; }

		constexpr void swap(native_file_handle &other) noexcept { std::swap(m_descriptor, other.m_descriptor); }
		friend constexpr void swap(native_file_handle &a, native_file_handle &b) noexcept { a.swap(b); }

	private:
		int m_descriptor = -1;
	};
}	 // namespace sek::system::detail