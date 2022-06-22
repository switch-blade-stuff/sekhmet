/*
 * Created by switchblade on 09/06/22
 */

#pragma once

#include <utility>

namespace sek::system::detail
{
	class native_file_handle
	{
		friend class native_filemap_handle;

	public:
		typedef int native_handle_type;

	public:
		constexpr native_file_handle() noexcept = default;

		SEK_API bool open(const char *path, openmode mode) noexcept;
		SEK_API bool close() noexcept;

		SEK_API std::int64_t read(void *dst, std::size_t n) const noexcept;
		SEK_API std::int64_t write(const void *src, std::size_t n) const noexcept;
		SEK_API std::int64_t seek(std::int64_t off, seek_dir way) const noexcept;
		inline std::int64_t tell() const noexcept { return this->seek(0, cur); }
		SEK_API bool sync() const noexcept;

		[[nodiscard]] constexpr bool is_open() const noexcept { return m_descriptor >= 0; }
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

		SEK_API bool map(const native_file_handle &file, std::int64_t off, std::int64_t n, mapmode mode) noexcept;
		SEK_API bool unmap() noexcept;

		[[nodiscard]] constexpr std::int64_t size() const noexcept { return m_data_size; }
		[[nodiscard]] constexpr void *data() const noexcept
		{
			return std::bit_cast<void *>(std::bit_cast<std::byte *>(m_handle) + m_data_offset);
		}

		[[nodiscard]] constexpr bool is_mapped() const noexcept { return m_handle != nullptr; }
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
		std::int64_t m_data_offset = 0; /* Offset from the handle to start of data. */
		std::int64_t m_data_size = 0;
	};
}	 // namespace sek::system::detail