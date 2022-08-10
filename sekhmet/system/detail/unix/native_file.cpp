/*
 * Created by switchblade on 09/06/22
 */

#include "native_file.hpp"

#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>

#if (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS >= 64) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
#define FTRUNCATE ::ftruncate
#define LSEEK ::lseek
#define MMAP ::mmap
#define OFF_T ::off_t
#elif defined(_LARGEFILE64_SOURCE)
#define FTRUNCATE ::ftruncate64
#define LSEEK ::lseek64
#define MMAP ::mmap64
#define OFF_T ::off64_t
#endif

#if (defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 16))) ||                            \
	(defined(_BSD_SOURCE) || defined(_XOPEN_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L))
#define USE_FSYNC
#endif

namespace sek::system::detail
{
	constexpr auto access = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	inline static unexpected<std::error_code> current_error() noexcept
	{
		return unexpected{std::make_error_code(std::errc{errno})};
	}
	inline static std::int64_t page_size() noexcept
	{
		const auto res = sysconf(_SC_PAGE_SIZE);
		return res < 0 ? SEK_KB(8) : static_cast<std::int64_t>(res);
	}

	expected<void, std::error_code> native_file_handle::open(const char *path, openmode mode) noexcept
	{
		/* Fail if already open. */
		if (is_open()) [[unlikely]]
			return unexpected{std::make_error_code(std::errc::invalid_argument)};

		const auto result = ::open(path, mode, access);
		if (result < 0) [[unlikely]]
			return current_error();
		m_descriptor = result;
		return {};
	}
	expected<void, std::error_code> native_file_handle::close() noexcept
	{
		if (::close(m_descriptor) != 0) [[unlikely]]
			return current_error();
		m_descriptor = -1;
		return {};
	}

	expected<void, std::error_code> native_file_handle::sync() const noexcept
	{
#ifdef USE_FSYNC
		if (::fsync(m_descriptor) != 0) [[unlikely]]
			return current_error();
#else
		::sync();
#endif
		return {};
	}

	expected<std::size_t, std::error_code> native_file_handle::read(void *dst, std::size_t n) const noexcept
	{
		if (const auto result = ::read(m_descriptor, dst, n); result >= 0) [[likely]]
			return static_cast<std::size_t>(result);
		return current_error();
	}
	expected<std::size_t, std::error_code> native_file_handle::write(const void *src, std::size_t n) const noexcept
	{
		if (const auto result = ::write(m_descriptor, src, n); result >= 0) [[likely]]
			return static_cast<std::size_t>(result);
		return current_error();
	}

	expected<std::uint64_t, std::error_code> native_file_handle::seek(std::int64_t off, seek_basis dir) const noexcept
	{
		const auto result = LSEEK(m_descriptor, static_cast<OFF_T>(off), dir);
		if (result < 0) [[unlikely]]
			return current_error();
		return static_cast<std::uint64_t>(result);
	}
	expected<std::uint64_t, std::error_code> native_file_handle::resize(std::uint64_t size) const noexcept
	{
		if (FTRUNCATE(m_descriptor, static_cast<OFF_T>(size)) != 0) [[unlikely]]
			return current_error();
		return size;
	}
	expected<std::uint64_t, std::error_code> native_file_handle::size() const noexcept
	{
		struct stat s;
		if (::fstat(m_descriptor, &s) != 0) [[unlikely]]
			return current_error();
		return static_cast<std::uint64_t>(s.st_size);
	}

	// clang-format off
	expected<void, std::error_code> native_filemap_handle::map(const native_file_handle &file, std::uint64_t off,
															   std::uint64_t n, openmode fm, mapmode mm) noexcept
	{
		if (is_mapped() || !file.is_open()) [[unlikely]]
			return unexpected{std::make_error_code(std::errc::invalid_argument)};

		const auto fd = file.native_handle();
		if (n == 0) [[unlikely]] /* If n is 0, map the entire file. */
		{
			const auto result = file.size();
			if (!result.has_value()) [[unlikely]]
				return result;
			n = *result;
		}

		/* Initialize protection mode & flags. */
		int prot = (fm & read_write) ? PROT_READ | PROT_WRITE : (fm & write_only) ? PROT_WRITE : PROT_READ;
		int flags = (mm & map_copy) ? MAP_PRIVATE : MAP_SHARED;
		if (mm & map_populate) flags |= MAP_POPULATE;

		const auto size_diff = off % page_size();
		const auto map_size = static_cast<std::size_t>(n + size_diff);
		const auto map_off = static_cast<OFF_T>(off - size_diff);

		const auto result = MMAP(nullptr, map_size, prot, flags, fd, map_off);
		if (result == MAP_FAILED) [[unlikely]]
			return current_error();

		m_handle = result;
		m_data_offset = size_diff;
		m_data_size = n;
		return {};
	}
	expected<void, std::error_code> native_filemap_handle::unmap() noexcept
	{
		if (!is_mapped()) [[unlikely]]
			return unexpected{std::make_error_code(std::errc::invalid_argument)};

		const auto total_size = static_cast<std::size_t>(m_data_size + m_data_offset);
		if (::munmap(std::exchange(m_handle, nullptr), total_size) != 0) [[unlikely]]
			return current_error();
		return {};
	}
	// clang-format on
}	 // namespace sek::system::detail
