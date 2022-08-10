/*
 * Created by switchblade on 09/06/22
 */

#pragma once

namespace sek::system::detail
{
	class native_file_handle
	{
		friend class native_filemap_handle;

	public:
		typedef void *native_handle_type;

	public:
		constexpr native_file_handle() noexcept = default;

		constexpr void open(native_handle_type handle) noexcept { m_handle = handle; }

		SEK_API expected<void, std::error_code> open(const char *path, openmode mode) noexcept;
		SEK_API expected<void, std::error_code> close() noexcept;

		SEK_API expected<void, std::error_code> sync() const noexcept;

		SEK_API expected<std::size_t, std::error_code> read(void *dst, std::size_t n) const noexcept;
		SEK_API expected<std::size_t, std::error_code> write(const void *src, std::size_t n) const noexcept;

		[[nodiscard]] SEK_API expected<std::uint64_t, std::error_code> seek(std::int64_t off, seek_basis dir) const noexcept;
		[[nodiscard]] expected<std::uint64_t, std::error_code> setpos(std::uint64_t pos) const noexcept
		{
			return seek(static_cast<std::int64_t>(pos), seek_set);
		}

		[[nodiscard]] SEK_API expected<std::uint64_t, std::error_code> resize(std::uint64_t size) const noexcept;

		[[nodiscard]] SEK_API expected<std::uint64_t, std::error_code> size() const noexcept;
		[[nodiscard]] expected<std::uint64_t, std::error_code> tell() const noexcept { return seek(0, seek_cur); }

		[[nodiscard]] constexpr bool is_open() const noexcept { return m_handle != nullptr; }

		[[nodiscard]] constexpr native_handle_type release() noexcept { return std::exchange(m_handle, nullptr); }
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_handle; }

		constexpr void swap(native_file_handle &other) noexcept { std::swap(m_handle, other.m_handle); }

	private:
		void *m_handle = nullptr;
	};
}	 // namespace sek::system::detail