/*
 * Created by switchblade on 12/09/22
 */

#include "native_filemap.hpp"

#include "native_file.hpp"
#include <sys/mman.h>

namespace sek::system::detail
{
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
			return unexpected{current_error()};

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
			return unexpected{current_error()};
		return {};
	}
	// clang-format on
}	 // namespace sek::system::detail