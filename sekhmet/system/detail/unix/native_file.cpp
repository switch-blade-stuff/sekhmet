/*
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
	inline static std::int64_t page_size() noexcept
	{
		const auto res = sysconf(_SC_PAGE_SIZE);
		return res < 0 ? SEK_KB(8) : static_cast<std::int64_t>(res);
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

	bool native_filemap_handle::map(const native_file_handle &file, std::int64_t off, std::int64_t n, openmode mode) noexcept
	{
		if (is_mapped() || !(file.is_open() && (mode & (in | out))) || (mode & (copy | out)) == copy) [[unlikely]]
			return false;

		const auto fd = file.m_descriptor;
		if (n == 0 && (n = file_size(file.m_descriptor)) < 0) [[unlikely]] /* If n is 0, map the entire file. */
			return false;

		/* Initialize protection mode & flags. */
		int prot = PROT_NONE;
		if (mode & in) prot |= PROT_READ;
		if (mode & out) prot |= PROT_WRITE;
		int flags = (mode & copy) ? MAP_PRIVATE : MAP_SHARED;
		if (mode & populate) flags |= MAP_POPULATE;

		const auto size_diff = off % page_size();
		const auto map_size = static_cast<std::size_t>(n + size_diff);
		const auto map_off = static_cast<OFF_T>(off - size_diff);
		if ((m_handle = MMAP(nullptr, map_size, prot, flags, fd, map_off)) == MAP_FAILED) [[unlikely]]
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
		if (is_mapped()) [[unlikely]]
			return false;
		return ::munmap(std::exchange(m_handle, nullptr), static_cast<std::size_t>(m_data_size + m_data_offset)) == 0;
	}
}	 // namespace sek::system::detail
