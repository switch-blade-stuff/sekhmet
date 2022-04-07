//
// Created by switchblade on 2022-04-07.
//

#include "filemap_handle.hpp"

#include <bit>
#include <fcntl.h>

#include "../assert.hpp"
#include <sys/mman.h>

namespace sek::detail
{
	static auto page_size() noexcept { return sysconf(_SC_PAGE_SIZE); }

	filemap_handle::native_handle_type filemap_handle::handle_from_view(void *ptr) noexcept
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(ptr);
		return std::bit_cast<void *>(int_ptr - (int_ptr % page_size()));
	}

	void filemap_handle::init(int fd, std::ptrdiff_t offset, std::size_t size, filemap_openmode mode, const char *)
	{
		auto offset_diff = offset % page_size(); /* Adjust offset to be a multiple of page size. */
		int prot = (mode & filemap_in ? PROT_READ : 0) | (mode & filemap_out ? PROT_WRITE : 0);

		view_ptr = mmap(nullptr, size + static_cast<std::size_t>(offset_diff), prot, MAP_SHARED, fd, offset - offset_diff);
		if (!view_ptr) [[unlikely]]
			throw filemap_error("Failed to mmap file");

		/* Offset might not be the same as the start position, need to adjust the pointer. */
		view_ptr = std::bit_cast<void *>(std::bit_cast<std::intptr_t>(view_ptr) + offset_diff);
		map_size = size;
	}
	filemap_handle::filemap_handle(const char *path, std::ptrdiff_t offset, std::size_t size, filemap_openmode mode, const char *name)
	{
		struct raii_file
		{
			constexpr explicit raii_file(int fd) noexcept : fd(fd) {}
			~raii_file() noexcept(false)
			{
				if (close(fd)) [[unlikely]]
					throw filemap_error("Failed to close file descriptor");
			}

			int fd;
		};

		int flags = O_RDONLY;
		if ((mode & filemap_in) && (mode & filemap_out))
			flags = O_RDWR;
		else if (mode & filemap_in)
			flags = O_WRONLY;
		auto file = raii_file{open(path, flags | O_CLOEXEC)};
		if (file.fd < 0) [[unlikely]]
			throw filemap_error("Failed to open file descriptor");

		init(file.fd, offset, size, mode, name);
	}
	filemap_handle::~filemap_handle()
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr);
		auto diff = int_ptr % page_size();
		SEK_ASSERT_ALWAYS(!munmap(std::bit_cast<void *>(int_ptr - diff), map_size + static_cast<std::size_t>(diff)));
	}
	void filemap_handle::flush(std::size_t n) const
	{
		auto int_ptr = std::bit_cast<std::intptr_t>(view_ptr);
		auto diff = int_ptr % page_size();
		if (msync(std::bit_cast<void *>(int_ptr - diff), n + static_cast<size_t>(diff), MS_SYNC | MS_INVALIDATE)) [[unlikely]]
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
}	 // namespace sek::detail
