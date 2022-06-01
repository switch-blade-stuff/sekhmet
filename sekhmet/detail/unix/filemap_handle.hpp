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
 * Created by switchblade on 2022-04-07
 */

#pragma once

namespace sek::detail
{
	class filemap_handle
	{
	public:
		using native_handle_type = void *;
		using native_file_type = int;

	private:
		SEK_API void init(native_file_type, std::ptrdiff_t, std::size_t, native_openmode, const char *);

	public:
		constexpr filemap_handle(filemap_handle &&other) noexcept
			: view_ptr(std::exchange(other.view_ptr, nullptr)),
			  map_size(std::exchange(other.map_size, 0)),
			  page_size(std::exchange(other.page_size, 0))
		{
		}
		constexpr filemap_handle &operator=(filemap_handle &&other) noexcept
		{
			swap(other);
			return *this;
		}

		filemap_handle(native_file_type fd, std::ptrdiff_t offset, std::size_t size, native_openmode mode, const char *name)
		{
			init(fd, offset, size, mode, name);
		}
		SEK_API filemap_handle(const char *path, std::ptrdiff_t offset, std::size_t size, native_openmode mode, const char *name);

		[[nodiscard]] constexpr std::size_t size() const noexcept { return map_size; }
		[[nodiscard]] constexpr void *data() const noexcept { return view_ptr; }

		SEK_API bool reset() noexcept;
		SEK_API void flush(std::ptrdiff_t off, std::ptrdiff_t n) const;

		[[nodiscard]] SEK_API native_handle_type native_handle() const noexcept;

		constexpr void swap(filemap_handle &other) noexcept
		{
			std::swap(view_ptr, other.view_ptr);
			std::swap(map_size, other.map_size);
			std::swap(page_size, other.page_size);
		}

	private:
		void *view_ptr = nullptr;
		std::size_t map_size = 0;
		std::ptrdiff_t page_size;
	};
}	 // namespace sek::detail
