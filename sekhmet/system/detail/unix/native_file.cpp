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

#include "native_file.hpp"

#include <fcntl.h>
#include <utility>

#include <sys/mman.h>
#include <sys/stat.h>

#if (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS >= 64) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
#define LSEEK ::lseek
#define MMAP ::mmap
#define OFF_T ::off_t
#elif defined(_LARGEFILE64_SOURCE)
#define LSEEK ::lseek64
#define MMAP ::mmap64
#define OFF_T ::off64_t
#endif

namespace sek::system::detail
{
	constexpr auto access = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	inline static std::int64_t file_size(int fd) noexcept
	{
		struct stat s;
		if (::fstat(fd, &s) < 0) [[unlikely]]
			return -1;
		return static_cast<std::int64_t>(s.st_size);
	}
	inline static std::size_t page_size() noexcept
	{
		const auto res = sysconf(_SC_PAGE_SIZE);
		return res < 0 ? SEK_KB(8) : static_cast<std::size_t>(res);
	}

	bool native_file_handle::open(const char *path, openmode mode) noexcept
	{
		if (is_open() || !(mode & (in | out))) [[unlikely]]
			return false;

		int native_flags;
		if ((mode & in) && (mode & out))
			native_flags = O_RDWR;
		else if (mode & in)
			native_flags = O_RDONLY;
		else if (mode & out)
			native_flags = O_WRONLY;
		if (mode & trunc) native_flags |= O_TRUNC;
		if (mode & append) native_flags |= O_APPEND;
		if (mode & create) native_flags |= O_CREAT;

		if ((m_descriptor = ::open(path, native_flags, access)) < 0) [[unlikely]]
			return false;

		/* Optionally seek file to the end. */
		if ((mode & atend) && LSEEK(m_descriptor, 0, SEEK_END) < 0) [[unlikely]]
		{
			::close(std::exchange(m_descriptor, -1));
			return false;
		}
		return true;
	}
	bool native_file_handle::close() noexcept { return ::close(std::exchange(m_descriptor, -1)) == 0; }

	std::int64_t native_file_handle::read(void *dst, std::size_t n) const noexcept
	{
		return ::read(m_descriptor, dst, n);
	}
	std::int64_t native_file_handle::write(const void *src, std::size_t n) const noexcept
	{
		return ::write(m_descriptor, src, n);
	}
	std::int64_t native_file_handle::seek(std::int64_t off, int way) const noexcept
	{
		return LSEEK(m_descriptor, static_cast<OFF_T>(off), way < 0 ? SEEK_SET : way > 0 ? SEEK_END : SEEK_CUR);
	}
	bool native_file_handle::sync() const noexcept
	{
#if (defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 16))) ||                            \
	(defined(_BSD_SOURCE) || defined(_XOPEN_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L))
		return ::fsync(m_descriptor) == 0;
#else
		::sync();
		return true;
#endif
	}

	bool native_filemap_handle::map(const native_file_handle &file, std::size_t off, std::size_t n, openmode mode) noexcept
	{
		if (is_mapped() || !(file.is_open() && (mode & (in | out))) || (mode & (copy | out)) == copy) [[unlikely]]
			return false;

		const auto fd = file.m_descriptor;
		if (n == 0) /* If n is 0, map the entire file. */
		{
			const auto max_size = file_size(file.m_descriptor);
			if (max_size < 0) [[unlikely]]
				return false;
			n = static_cast<std::size_t>(max_size);
		}

		/* Initialize protection mode & flags. */
		int prot = PROT_NONE;
		if (mode & in) prot |= PROT_READ;
		if (mode & out) prot |= PROT_WRITE;
		int flags = (mode & copy) ? MAP_PRIVATE : MAP_SHARED;
		if (mode & populate) flags |= MAP_POPULATE;

		const auto size_diff = off % page_size();
		if ((m_handle = MMAP(nullptr, n + size_diff, prot, flags, fd, static_cast<OFF_T>(off - size_diff))) == MAP_FAILED)
		{
			m_handle = nullptr;
			return false;
		}

		m_data_offset = size_diff;
		m_data_size = n;
		return true;
	}
	bool native_filemap_handle::unmap() noexcept
	{
		return is_mapped() && ::munmap(std::exchange(m_handle, nullptr), m_data_size + m_data_offset) == 0;
	}
}	 // namespace sek::system::detail
