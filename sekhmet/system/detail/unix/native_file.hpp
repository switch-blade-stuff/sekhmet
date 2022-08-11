/*
 * Created by switchblade on 09/06/22
 */

#pragma once

namespace sek::system::detail
{
	class native_file_handle
	{
	public:
		typedef int native_handle_type;

	public:
		constexpr native_file_handle() noexcept = default;
		~native_file_handle();

		constexpr void open(native_handle_type handle) noexcept { m_descriptor = handle; }

		expected<void, std::error_code> open(const char *path, openmode mode) noexcept;
		expected<void, std::error_code> close() noexcept;

		expected<void, std::error_code> sync() const noexcept;

		expected<std::size_t, std::error_code> read(void *dst, std::size_t n) const noexcept;
		expected<std::size_t, std::error_code> write(const void *src, std::size_t n) const noexcept;

		[[nodiscard]] expected<std::uint64_t, std::error_code> seek(std::int64_t off, seek_basis dir) const noexcept;
		[[nodiscard]] expected<std::uint64_t, std::error_code> setpos(std::uint64_t pos) const noexcept
		{
			return seek(static_cast<std::int64_t>(pos), seek_set);
		}

		[[nodiscard]] expected<std::uint64_t, std::error_code> resize(std::uint64_t size) const noexcept;

		[[nodiscard]] expected<std::uint64_t, std::error_code> size() const noexcept;
		[[nodiscard]] expected<std::uint64_t, std::error_code> tell() const noexcept { return seek(0, seek_cur); }

		[[nodiscard]] constexpr bool is_open() const noexcept { return m_descriptor >= 0; }

		[[nodiscard]] constexpr native_handle_type release() noexcept { return std::exchange(m_descriptor, -1); }
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_descriptor; }

		constexpr void swap(native_file_handle &other) noexcept { std::swap(m_descriptor, other.m_descriptor); }

	private:
		int m_descriptor = -1;
	};

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
}	 // namespace sek::system::detail