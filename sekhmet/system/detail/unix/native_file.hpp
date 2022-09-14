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
}	 // namespace sek::system::detail