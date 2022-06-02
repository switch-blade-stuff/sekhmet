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
 * Created by switchblade on 2022-04-07
 */

#include "filemap_handle.hpp"

#include <bit>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>

namespace sek::detail
{
	static std::size_t file_size(int fd) noexcept
	{
		struct stat st;
		return !fstat(fd, &st) ? static_cast<std::size_t>(st.st_size) : 0;
	}

	void filemap_handle::init(int fd, std::ptrdiff_t offset, std::size_t size, native_openmode mode, const char *)
	{
		page_size = sysconf(_SC_PAGE_SIZE);

		int prot = (mode & native_in ? PROT_READ : 0) | (mode & native_out ? PROT_WRITE : 0);
		int flags = mode & native_copy ? MAP_PRIVATE : MAP_SHARED;

		/* Adjust offset to be a multiple of page size. */
		auto offset_diff = offset % page_size;
		auto real_offset = offset - offset_diff;

		/* Get the actual size from the file descriptor if size == 0. */
		std::size_t real_size;
		if (!size) [[unlikely]]
		{
			if ((real_size = file_size(fd)) == 0) [[unlikely]]
				throw filemap_error("Failed to get file size");
			real_size = (size = real_size - static_cast<std::size_t>(offset)) + static_cast<std::size_t>(offset_diff);
		}
		else
			real_size = size + static_cast<std::size_t>(offset_diff);

		view_ptr = mmap(nullptr, real_size, prot, flags, fd, real_offset);
		if (!view_ptr) [[unlikely]]
			throw filemap_error("Failed to mmap file");

		/* Offset might not be the same as the start position, need to adjust the pointer. */
		view_ptr = std::bit_cast<void *>(std::bit_cast<std::intptr_t>(view_ptr) + offset_diff);
		map_size = size;
	}
	filemap_handle::filemap_handle(const char *path, std::ptrdiff_t offset, std::size_t size, native_openmode mode, const char *name)
	{
		struct raii_fd
		{
			constexpr explicit raii_fd(int fd) noexcept : fd(fd) {}
			~raii_fd() { close(fd); }

			int fd;
		};

		int flags = O_RDONLY;
		if ((mode & native_in) && (mode & native_out))
			flags = O_RDWR;
		else if (mode & native_in)
			flags = O_WRONLY;
		auto file = raii_fd{open(path, flags | O_CLOEXEC)};
		if (file.fd < 0) [[unlikely]]
			throw filemap_error("Failed to open file descriptor");

		init(file.fd, offset, size, mode, name);
	}
	bool filemap_handle::reset() noexcept
	{
		if (view_ptr) [[likely]]
		{
			auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr);
			auto diff = int_ptr % page_size;
			return !munmap(std::bit_cast<void *>(int_ptr - diff), map_size + static_cast<std::size_t>(diff));
		}
		return false;
	}
	void filemap_handle::flush(std::ptrdiff_t off, std::ptrdiff_t n) const
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr) + off;
		auto diff = int_ptr % page_size;
		if (msync(std::bit_cast<void *>(int_ptr - diff), static_cast<std::size_t>(n + diff), MS_SYNC | MS_INVALIDATE))
			[[unlikely]]
		{
			switch (errno)
			{
				case EBUSY: throw filemap_error("Mapped file is busy");
				case ENOMEM:
				case EINVAL: throw filemap_error("Bad mapping handle");
				default: throw filemap_error("Call to `msync` failed");
			}
		}
	}

	filemap_handle::native_handle_type filemap_handle::native_handle() const noexcept
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr);
		return std::bit_cast<void *>(int_ptr - (int_ptr % page_size));
	}
}	 // namespace sek::detail
