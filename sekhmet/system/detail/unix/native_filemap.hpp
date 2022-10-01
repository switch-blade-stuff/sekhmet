/*
 * Created by switchblade on 09/06/22
 */

#pragma once

namespace sek::detail
{
	class native_filemap_handle
	{
	public:
		typedef void *native_handle_type;

	public:
		constexpr native_filemap_handle() noexcept = default;

		// clang-format off
		SEK_API expected<void, std::error_code> map(const native_file_handle &file, std::uint64_t off,
													std::uint64_t n, openmode fm, mapmode mm) noexcept;
		SEK_API expected<void, std::error_code> unmap() noexcept;
		// clang-format on

		[[nodiscard]] constexpr std::uint64_t size() const noexcept { return m_data_size; }
		[[nodiscard]] constexpr void *data() const noexcept
		{
			const auto bytes = std::bit_cast<std::byte *>(m_handle);
			return std::bit_cast<void *>(bytes + m_data_offset);
		}

		[[nodiscard]] constexpr bool is_mapped() const noexcept { return m_handle != nullptr; }

		[[nodiscard]] constexpr native_handle_type release() noexcept { return std::exchange(m_handle, nullptr); }
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_handle; }

		constexpr void swap(native_filemap_handle &other) noexcept
		{
			std::swap(m_handle, other.m_handle);
			std::swap(m_data_offset, other.m_data_offset);
			std::swap(m_data_size, other.m_data_size);
		}

	private:
		/* The handle and the data may point to different memory locations, since mmap requires page alignment. */
		native_handle_type m_handle = nullptr;
		std::uint64_t m_data_offset = 0; /* Offset from the handle to start of data. */
		std::uint64_t m_data_size = 0;
	};
}	 // namespace sek::detail