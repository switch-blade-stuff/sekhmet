/*
 * Created by switchblade on 09/06/22
 */

#include "native_file.hpp"

#include <fcntl.h>

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

	inline static std::error_code current_error() noexcept { return std::make_error_code(std::errc{errno}); }
	inline static std::uint64_t page_size() noexcept
	{
		const auto res = sysconf(_SC_PAGE_SIZE);
		return res < 0 ? SEK_KB(8) : static_cast<std::uint64_t>(res);
	}

	native_file_handle::~native_file_handle()
	{
		if (m_descriptor >= 0) [[likely]]
			::close(m_descriptor);
	}

	expected<void, std::error_code> native_file_handle::open(const char *path, openmode mode) noexcept
	{
		/* Fail if already open. */
		if (is_open()) [[unlikely]]
			return unexpected{std::make_error_code(std::errc::invalid_argument)};

		const auto result = ::open(path, mode, access);
		if (result < 0) [[unlikely]]
			return unexpected{current_error()};
		m_descriptor = result;
		return {};
	}
	expected<void, std::error_code> native_file_handle::close() noexcept
	{
		if (::close(m_descriptor) != 0) [[unlikely]]
			return unexpected{current_error()};
		m_descriptor = -1;
		return {};
	}

	expected<void, std::error_code> native_file_handle::sync() const noexcept
	{
#ifdef USE_FSYNC
		if (::fsync(m_descriptor) != 0) [[unlikely]]
			return unexpected{current_error()};
#else
		::sync();
#endif
		return {};
	}

	expected<std::size_t, std::error_code> native_file_handle::read(void *dst, std::size_t n) const noexcept
	{
		if (const auto result = ::read(m_descriptor, dst, n); result >= 0) [[likely]]
			return static_cast<std::size_t>(result);
		return unexpected{current_error()};
	}
	expected<std::size_t, std::error_code> native_file_handle::write(const void *src, std::size_t n) const noexcept
	{
		if (const auto result = ::write(m_descriptor, src, n); result >= 0) [[likely]]
			return static_cast<std::size_t>(result);
		return unexpected{current_error()};
	}

	expected<std::uint64_t, std::error_code> native_file_handle::seek(std::int64_t off, seek_basis dir) const noexcept
	{
		const auto result = LSEEK(m_descriptor, static_cast<OFF_T>(off), dir);
		if (result < 0) [[unlikely]]
			return unexpected{current_error()};
		return static_cast<std::uint64_t>(result);
	}
	expected<std::uint64_t, std::error_code> native_file_handle::resize(std::uint64_t size) const noexcept
	{
		if (FTRUNCATE(m_descriptor, static_cast<OFF_T>(size)) != 0) [[unlikely]]
			return unexpected{current_error()};
		return size;
	}
	expected<std::uint64_t, std::error_code> native_file_handle::size() const noexcept
	{
		struct stat s;
		if (::fstat(m_descriptor, &s) != 0) [[unlikely]]
			return unexpected{current_error()};
		return static_cast<std::uint64_t>(s.st_size);
	}
}	 // namespace sek::system::detail
