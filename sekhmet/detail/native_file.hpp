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
 * Created by switchblade on 30/05/22
 */

#pragma once

#include <filesystem>

#include "assert.hpp"
#include "define.h"
#include "native_util.hpp"

namespace sek::detail
{
	class native_file_handle;
}

#ifdef SEK_OS_WIN
#include "win/native_file_handle.hpp"
#elif defined(SEK_OS_UNIX)
#include "unix/native_file_handle.hpp"
#endif

namespace sek::detail
{
	class native_file
	{
		using handle_t = native_file_handle;

	public:
		typedef typename handle_t::native_type native_type;
		typedef detail::native_openmode openmode;

		/** Enables read mode for the filemap. */
		constexpr static openmode in = detail::native_in;
		/** Enables write mode for the filemap. */
		constexpr static openmode out = detail::native_out;
		/** Enables copy-on-write mode for the filemap. Implies `out`. */
		constexpr static openmode copy = detail::native_copy | out;

	public:
		constexpr native_file(native_file &&other) noexcept : handle(std::move(other.handle)) {}
		constexpr native_file &operator=(native_file &&other) noexcept
		{
			handle = std::move(other.handle);
			return *this;
		}
		~native_file() { SEK_ASSERT_ALWAYS(handle.close()); }

		explicit native_file(const std::filesystem::path &path, openmode mode = in) { open(path, mode); }
		explicit native_file(std::basic_string_view<std::filesystem::path::value_type> path, openmode mode = in)
		{
			open(path, mode);
		}
		explicit native_file(const typename std::filesystem::path::value_type *path, openmode mode = in)
		{
			open(path, mode);
		}

		[[nodiscard]] constexpr bool is_open() const noexcept { return handle.is_open(); }
		[[nodiscard]] constexpr native_type native_handle() const noexcept { return handle.native_handle(); }

		bool open(const std::filesystem::path &path, openmode mode) { return open(path.c_str(), mode); }
		bool open(std::basic_string_view<std::filesystem::path::value_type> path, openmode mode)
		{
			return open(path.data(), mode);
		}
		bool open(const typename std::filesystem::path::value_type *path, openmode mode)
		{
			return handle.open(path, mode);
		}
		bool close() { return handle.close(); }

		[[nodiscard]] std::size_t write(const void *src, std::size_t n) { return handle.write(src, n); }
		[[nodiscard]] std::size_t read(void *dst, std::size_t n) { return handle.read(dst, n); }
		[[nodiscard]] ssize_t seek(ssize_t pos, int way /* -1 - begin, 0 - current, 1 - end */)
		{
			return handle.seek(pos, way);
		}
		[[nodiscard]] bool sync() { return handle.sync(); }

		constexpr void swap(native_file &other) noexcept { handle.swap(other.handle); }

	private:
		handle_t handle;
	};
}	 // namespace sek::detail