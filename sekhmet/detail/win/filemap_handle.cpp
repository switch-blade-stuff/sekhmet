//
// Created by switchblade on 2022-04-07.
//

#include "filemap_handle.hpp"

#include <limits>
#include <windows.h>

#include "../debug.hpp"

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

	static std::ptrdiff_t alloc_granularity() noexcept
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		return static_cast<std::ptrdiff_t>(info.dwAllocationGranularity);
	}
	void filemap_handle::init(void *fd, std::ptrdiff_t offset, std::ptrdiff_t size, int mode, const char *name)
	{
		auto mapping = create_mapping(fd, name);
		if (!mapping) [[unlikely]]
			throw filemap_error("Failed to create file mapping object");

		/* View must start at a multiple of allocation granularity. */
		auto offset_diff = offset % alloc_granularity();
		auto real_offset = offset - offset_diff;
		auto real_size = size + offset_diff;

		DWORD access = mode & filemap_out ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
		ULARGE_INTEGER offset_qword = {.QuadPart = static_cast<ULONGLONG>(real_offset)};

		view_ptr = MapViewOfFile(mapping, access, offset_qword.HighPart, offset_qword.LowPart, static_cast<DWORD>(real_size));
		if (!view_ptr) [[unlikely]]
			throw filemap_error("Failed to map view of file");

		/* Offset might not be the same as the start position, need to adjust the handle pointer. */
		view_ptr = std::bit_cast<HANDLE>(std::bit_cast<std::intptr_t>(view_ptr) + offset_diff);
		map_size = size;
	}
	filemap_handle::filemap_handle(const wchar_t *path, std::ptrdiff_t offset, std::ptrdiff_t size, int mode, const char *name)
	{
		struct raii_file
		{
			constexpr explicit raii_file(HANDLE ptr) noexcept : ptr(ptr) {}
			~raii_file() noexcept(false)
			{
				if (!CloseHandle(ptr)) [[unlikely]]
					throw filemap_error("Failed to close file handle");
			}

			HANDLE ptr;
		};

		DWORD access = GENERIC_READ, share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (mode & filemap_out) access |= GENERIC_WRITE;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
		auto file = raii_file{CreateFile2(path, access, share, OPEN_EXISTING, nullptr)};
#else
		auto file = raii_file{CreateFileW(path, access, share, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
#endif
		if (!file.ptr) [[unlikely]]
			throw filemap_error("Failed to create file handle");
		init(file.ptr, offset, size, mode, name);
	}

	filemap_handle::native_handle_type filemap_handle::handle_from_view(void *ptr) noexcept
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(ptr);
		return std::bit_cast<HANDLE>(int_ptr - (int_ptr % alloc_granularity()));
	}

	filemap_handle::~filemap_handle() { SEK_ASSERT_ALWAYS(UnmapViewOfFile(native_handle())); }
	void filemap_handle::flush(std::ptrdiff_t n) const
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr);
		auto diff = int_ptr % alloc_granularity();

		if (!FlushViewOfFile(std::bit_cast<HANDLE>(int_ptr - diff), static_cast<SIZE_T>(n + diff))) [[unlikely]]
			throw filemap_error("`FlushViewOfFile` returned 0");
	}
}	 // namespace sek::detail
