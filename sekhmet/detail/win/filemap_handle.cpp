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

#include "filemap_handle.hpp"

#include <limits>
#include <windows.h>

namespace sek::detail
{
	static void *create_mapping(HANDLE fd, const char *name)
	{
		/* Convert name to wide string. */
		std::unique_ptr<wchar_t[]> wide_name;
		if (name)
		{
			auto buff_len = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, name, -1, nullptr, 0);
			if (buff_len > 0 && (wide_name = std::make_unique<wchar_t[]>(buff_len + 1))) [[likely]]
			{
				if (MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, name, -1, wide_name.get(), buff_len) <= 0) [[unlikely]]
					wide_name.reset();
			}
		}

		return CreateFileMappingW(fd, nullptr, PAGE_READWRITE, 0, 0, wide_name.get());
	}
	void filemap_handle::init(void *fd, std::ptrdiff_t offset, std::size_t size, int mode, const char *name)
	{
		struct raii_mapping
		{
			constexpr explicit raii_mapping(HANDLE ptr) noexcept : ptr(ptr) {}
			~raii_mapping() { CloseHandle(ptr); }

			HANDLE ptr;
		};

		/* Initialize view alignment. */
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		alignment = static_cast<std::ptrdiff_t>(info.dwAllocationGranularity);

		/* View must start at a multiple of allocation granularity. */
		auto offset_diff = static_cast<std::ptrdiff_t>(offset % alignment);
		ULARGE_INTEGER real_offset = {.QuadPart = static_cast<ULONGLONG>(offset - offset_diff)};

		/* If size == 0, get size of the entire file. */
		LARGE_INTEGER real_size;
		if (!size) [[unlikely]]
		{
			if (!GetFileSizeEx(fd, &real_size)) [[unlikely]]
				throw filemap_error("Failed to get file size");
			real_size.QuadPart =
				static_cast<LONGLONG>(size = static_cast<std::size_t>(real_size.QuadPart - offset)) + offset_diff;
		}
		else
			real_size.QuadPart = static_cast<LONGLONG>(size) + offset_diff;

		/* Create temporary mapping object for the specified name. */
		auto mapping = raii_mapping{create_mapping(fd, name)};
		if (!mapping.ptr) [[unlikely]]
			throw filemap_error("Failed to create file mapping object");

		DWORD access = mode & native_in ? FILE_MAP_READ : 0;
		if (mode & native_copy)
			access |= FILE_MAP_COPY;
		else if (mode & native_out)
			access |= FILE_MAP_WRITE;

		view_ptr = MapViewOfFile(
			mapping.ptr, access, real_offset.HighPart, real_offset.LowPart, static_cast<SIZE_T>(real_size.QuadPart));
		if (!view_ptr) [[unlikely]]
			throw filemap_error("Failed to map view of file");

		/* Offset might not be the same as the start position, need to adjust the handle pointer. */
		view_ptr = std::bit_cast<void *>(std::bit_cast<std::intptr_t>(view_ptr) + offset_diff);
		map_size = size;
	}
	filemap_handle::filemap_handle(const wchar_t *path, std::ptrdiff_t offset, std::size_t size, int mode, const char *name)
	{
		struct raii_file
		{
			constexpr explicit raii_file(HANDLE ptr) noexcept : ptr(ptr) {}
			~raii_file() { CloseHandle(ptr); }

			HANDLE ptr;
		};

		DWORD access = GENERIC_READ, share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (mode & native_out) access |= GENERIC_WRITE;

		auto file = raii_file{CreateFileW(
			path, access, share, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS | FILE_FLAG_NO_BUFFERING, nullptr)};
		if (!file.ptr) [[unlikely]]
			throw filemap_error("Failed to create file handle");
		init(file.ptr, offset, size, mode, name);
	}

	bool filemap_handle::reset() noexcept
	{
		if (view_ptr) [[likely]]
			return UnmapViewOfFile(native_handle());
		return false;
	}
	void filemap_handle::flush(std::ptrdiff_t off, std::ptrdiff_t n) const
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr) + off;
		auto diff = int_ptr % alignment;

		if (!FlushViewOfFile(std::bit_cast<HANDLE>(int_ptr - diff), static_cast<SIZE_T>(n + diff))) [[unlikely]]
			throw filemap_error("`FlushViewOfFile` returned 0");
	}

	filemap_handle::native_handle_type filemap_handle::native_handle() const noexcept
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr);
		return std::bit_cast<HANDLE>(int_ptr - (int_ptr % alignment));
	}
}	 // namespace sek::detail
