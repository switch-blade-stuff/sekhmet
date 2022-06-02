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
 * Created by switchblade on 30/05/22
 */

#include "native_file_handle.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <utility>

#define ASSERT_OPEN SEK_ASSERT_ALWAYS(is_open(), "File must be open")
#define ASSERT_CLOSED SEK_ASSERT_ALWAYS(!is_open(), "File must not be open")

namespace sek::detail
{
	bool native_file_handle::open(const char *path, native_openmode mode)
	{
		ASSERT_CLOSED;

		constexpr auto prot_flags = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
		auto mode_flags = mode == (native_in | native_out) ? O_RDWR : mode & native_out ? O_WRONLY : O_RDONLY;

		if (mode & native_append) mode_flags |= O_APPEND;
		if (mode & native_create) mode_flags |= O_CREAT;
		if (mode & native_trunc) mode_flags |= O_TRUNC;

		return (fd = ::open(path, mode_flags, prot_flags)) >= 0;
	}
	bool native_file_handle::close() { return ::close(std::exchange(fd, -1)) == 0; }

	std::size_t native_file_handle::write(const void *src, std::size_t n)
	{
		ASSERT_OPEN;
		const auto res = ::write(fd, src, n);
		return static_cast<size_t>(res < 0 ? 0 : res);
	}
	std::size_t native_file_handle::read(void *dst, std::size_t n)
	{
		ASSERT_OPEN;
		const auto res = ::read(fd, dst, n);
		return static_cast<size_t>(res < 0 ? 0 : res);
	}
	ssize_t native_file_handle::seek(ssize_t pos, int way)
	{
		ASSERT_OPEN;

		way = way < 0 ? SEEK_SET : way == 0 ? SEEK_CUR : SEEK_END;
#if defined(_POSIX_C_SOURCE) && defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS < 64
		return ::lseek64(fd, pos, way);
#else
		return ::lseek(fd, pos, way);
#endif
	}
	bool native_file_handle::sync()
	{
#ifdef _GNU_SOURCE
		ASSERT_OPEN;
		return ::syncfs(fd) == 0;
#else
		::sync();
		return true;
#endif
	}
}	 // namespace sek::detail