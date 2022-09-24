/*
 * Created by switchblade on 09/06/22
 */

#pragma once

#include "native_file.hpp"

#if defined(SEK_OS_UNIX)
#include "detail/unix/native_filemap.hpp"
#else
#error "Native file not implemented"
#endif

namespace sek::system
{
	/** @brief Structure used to manage a memory-mapped file. */
	class native_filemap
	{
	public:
		typedef typename detail::native_filemap_handle::native_handle_type native_handle_type;

		typedef system::mapmode mapmode;
		/** Enable copy-on-write for mapped pages. Any changes will not be committed to the backing file.
		 * @note Source file must be open for reading. */
		constexpr static mapmode map_copy = system::map_copy;
		/** Pre-populate mapped pages. */
		constexpr static mapmode map_populate = system::map_populate;

	public:
		native_filemap(const native_filemap &) = delete;
		native_filemap &operator=(const native_filemap &) = delete;

		/** Initializes an invalid (not mapped) filemap. */
		constexpr native_filemap() noexcept = default;
		inline ~native_filemap() { unmap(); }

		constexpr native_filemap(native_filemap &&other) noexcept { swap(other); }
		constexpr native_filemap &operator=(native_filemap &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** @brief Maps a portion of the file into memory.
		 * @param file File to create a mapping of.
		 * @param off Offset into the file at which to create the mapping.
		 * @param n Amount of bytes to map. Must be less than file size - offset. If set to 0, maps the entire file.
		 * @param mode Mode of the mapping. If set to 0 will use the default mode.
		 * @note After a file has been mapped, the source file can be closed.
		 * @note File should be open with a combination of `in` and `out` modes.
		 * @throw std::system_error On implementation-defined system errors. */
		explicit native_filemap(const native_file &file, std::uint64_t off = 0, std::uint64_t n = 0, mapmode mode = 0)
		{
			map(file, off, n, mode);
		}

		// clang-format off
		/** @copydoc native_filemap */
		SEK_API void map(const native_file &file, std::uint64_t off = 0, std::uint64_t n = 0, mapmode mode = 0);
		/** @copybrief map
		 * @param file File to create a mapping of.
		 * @param off Offset into the file at which to create the mapping.
		 * @param n Amount of bytes to map. Must be less than file size - offset. If set to 0, maps the entire file.
		 * @param mode Mode of the mapping. If set to 0 will use the default mode.
		 * @return `void` or an error code.
		 * @note After a file has been mapped, the source file can be closed.
		 * @note File should be open with a combination of `in` and `out` modes. */
		expected<void, std::error_code> map(std::nothrow_t, const native_file &file, std::uint64_t off = 0, std::uint64_t n = 0, mapmode mode = 0) noexcept
		{
			return m_handle.map(file.m_handle, off, n, file.mode(), mode);
		}
		// clang-format on

		/** @brief Unmaps the file mapped file from memory.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API void unmap();
		/** @copybrief unmap
		 * @return `void` or an error code. */
		inline expected<void, std::error_code> unmap(std::nothrow_t) noexcept { return m_handle.unmap(); }

		/** Returns size of the mapping. */
		[[nodiscard]] constexpr std::uint64_t size() const noexcept { return m_handle.size(); }
		/** Returns pointer to the mapped data. */
		[[nodiscard]] constexpr void *data() const noexcept { return m_handle.data(); }
		/** Returns span of bytes to the mapped data. */
		[[nodiscard]] constexpr std::span<std::byte> bytes() const noexcept
		{
			return {static_cast<std::byte *>(data()), static_cast<std::size_t>(size())};
		}
		/** Returns ASIO buffer of the mapped data. */
		[[nodiscard]] asio::mutable_buffer buffer() const noexcept { return asio::buffer(data(), size()); }

		/** @brief Checks if the mapping is valid. */
		[[nodiscard]] constexpr bool is_mapped() const noexcept { return m_handle.is_mapped(); }

		/** Releases and returns the underlying OS handle of the file mapping. */
		[[nodiscard]] constexpr native_handle_type release() noexcept { return m_handle.release(); }
		/** Returns underlying OS handle of the file mapping. */
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_handle.native_handle(); }

		constexpr void swap(native_filemap &other) noexcept { m_handle.swap(other.m_handle); }
		friend constexpr void swap(native_filemap &a, native_filemap &b) noexcept { a.swap(b); }

	private:
		detail::native_filemap_handle m_handle;
	};
}	 // namespace sek::system
