//
// Created by switchblade on 2022-04-07.
//

#pragma once

#include <filesystem>
#include <span>
#include <stdexcept>
#include <utility>

#include "assert.hpp"
#include "define.h"
#include "native_util.hpp"

namespace sek
{
	/** @brief Exception thrown by the `filemap` class on implementation-defined errors. */
	class filemap_error : public std::runtime_error
	{
	public:
		filemap_error() : std::runtime_error("Unknown filemap error") {}
		explicit filemap_error(const char *msg) : std::runtime_error(msg) {}
		~filemap_error() override = default;
	};

	namespace detail
	{
		class filemap_handle;
	}	 // namespace detail
}	 // namespace sek

#ifdef SEK_OS_WIN
#include "win/filemap_handle.hpp"
#elif defined(SEK_OS_UNIX)
#include "unix/filemap_handle.hpp"
#endif

namespace sek
{
	/** @brief Structure used to create & work with memory-mapped files. */
	class filemap
	{
	public:
		typedef typename detail::filemap_handle::native_file_type native_file_type;
		typedef typename detail::filemap_handle::native_handle_type native_handle_type;

		typedef detail::native_openmode openmode;

		/** Enables read mode for the filemap. */
		constexpr static openmode in = detail::native_in;
		/** Enables write mode for the filemap. */
		constexpr static openmode out = detail::native_out;
		/** Enables copy-on-write mode for the filemap. Implies `out`. */
		constexpr static openmode copy = detail::native_copy | out;

	public:
		filemap() = delete;
		filemap(const filemap &) = delete;
		filemap &operator=(const filemap &) = delete;

		constexpr filemap(filemap &&other) noexcept : handle(std::move(other.handle)), map_mode(other.map_mode) {}
		constexpr filemap &operator=(filemap &&other) noexcept
		{
			handle = std::move(other.handle);
			map_mode = other.map_mode;
			return *this;
		}
		~filemap() { SEK_ASSERT_ALWAYS(handle.reset()); }

		/** Initializes a filemap for the specified file using a size and an offset.
		 * @param path Path of the file to map into memory.
		 * @param offset Offset into the file (in bytes) to start the mapping at.
		 * @param size Amount of bytes from the offset position to map into memory. If set to 0, maps the entire file.
		 * @param mode Mode of the file map. By default, file mappings are read-only.
		 * @param name Optional name for the file mapping. If the OS does not support named file mapping, the name is ignored.
		 * @throw filemap_error On any implementation-defined error.
		 * @note If size is set to 0, maps the entire file. */
		explicit filemap(const std::filesystem::path &path,
						 std::ptrdiff_t offset = 0,
						 std::size_t size = 0,
						 openmode mode = in,
						 const char *name = nullptr)
			: handle(path.c_str(), offset, size, mode, name), map_mode(mode)
		{
		}
		// clang-format on
		/** @copydoc filemap */
		explicit filemap(std::basic_string_view<std::filesystem::path::value_type> path,
						 std::ptrdiff_t offset = 0,
						 std::size_t size = 0,
						 openmode mode = in,
						 const char *name = nullptr)
			: handle(path.data(), offset, size, mode, name), map_mode(mode)
		{
		}

		/** Initializes a filemap for the specified native file descriptor type using a size and an offset.
		 * @param fd Native file descriptor to create the mapping for.
		 * @param offset Offset into the file (in bytes) to start the mapping at.
		 * @param size Amount of bytes from the offset position to map into memory. If set to 0, maps the entire file.
		 * @param mode Mode of the file map. By default, file mappings are read-only.
		 * @param name Optional name for the file mapping. If the OS does not support named file mapping, the name is ignored.
		 * @throw filemap_error On any implementation-defined error. */
		explicit filemap(native_file_type fd, std::ptrdiff_t offset = 0, std::size_t size = 0, openmode mode = in, const char *name = nullptr)
			: handle(fd, offset, size, mode, name), map_mode(mode)
		{
		}

		/** Returns the mode of the file mapping. */
		[[nodiscard]] constexpr openmode mode() const noexcept { return map_mode; }

		/** Returns the size (in bytes) of the file mapping. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return handle.size(); }
		/** Returns pointer to the start of the mapped file. */
		[[nodiscard]] constexpr void *data() const noexcept { return handle.data(); }

		/** Returns an `std::byte` span to the mapped memory. */
		[[nodiscard]] constexpr std::span<std::byte> bytes() const noexcept
		{
			return std::span<std::byte>{static_cast<std::byte *>(data()), static_cast<std::size_t>(size())};
		}

		/** Flushes portion of the mapped memory to it's backing file.
		 * @param off Offset into the mapping at which to start the flush.
		 * @param n Amount of bytes to flush.
		 * @throw filemap_error On any implementation-defined error.
		 * @note pos + n must not exceed the size of the mapping. */
		void flush(std::ptrdiff_t pos = 0, std::ptrdiff_t n = -1) const
		{
			if (n < 0) n = static_cast<std::ptrdiff_t>(size()) - pos;

			SEK_ASSERT(pos > 0 && n > 0);
			SEK_ASSERT(pos + n <= static_cast<std::ptrdiff_t>(size()));

			handle.flush(pos, n);
		}

		/** Returns the underlying native handle. */
		[[nodiscard]] native_handle_type native_handle() const noexcept { return handle.native_handle(); }

		constexpr void swap(filemap &other) noexcept
		{
			handle.swap(other.handle);
			std::swap(map_mode, other.map_mode);
		}
		friend constexpr void swap(filemap &a, filemap &b) noexcept { a.swap(b); }

	private:
		detail::filemap_handle handle;
		openmode map_mode;
	};
}	 // namespace sek