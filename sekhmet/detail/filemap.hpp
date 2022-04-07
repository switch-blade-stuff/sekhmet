//
// Created by switchblade on 2022-04-07.
//

#pragma once

#include <filesystem>
#include <span>
#include <stdexcept>

#include "define.h"

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
		typedef int filemap_openmode;

		constexpr auto filemap_in = static_cast<filemap_openmode>(0b1);
		constexpr auto filemap_out = static_cast<filemap_openmode>(0b10);

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

		typedef detail::filemap_openmode openmode;

		constexpr static openmode in = detail::filemap_in;
		constexpr static openmode out = detail::filemap_out;

	public:
		/** Initializes a filemap for the specified file using a size and an offset.
		 * @param file_path Path of the file to map into memory.
		 * @param offset Offset into the file (in bytes) to start the mapping at.
		 * @param size Amount of bytes from the offset position to map into memory.
		 * @param mode Mode of the file map. By default, file mappings are read-only.
		 * @param name Optional name for the file mapping. If the OS does not support named file mapping, the name is ignored.
		 * @throw filemap_error On any implementation-defined error. */
		filemap(const std::filesystem::path &file_path,
				std::ptrdiff_t offset,
				std::size_t size,
				openmode mode = in,
				const char *name = nullptr)
			: handle(file_path.c_str(), offset, size, mode, name)
		{
		}
		/** Initializes a filemap for the specified native file descriptor type using a size and an offset.
		 * @param fd Native file descriptor to create the mapping for.
		 * @param offset Offset into the file (in bytes) to start the mapping at.
		 * @param size Amount of bytes from the offset position to map into memory.
		 * @param mode Mode of the file map. By default, file mappings are read-only.
		 * @param name Optional name for the file mapping. If the OS does not support named file mapping, the name is ignored.
		 * @throw filemap_error On any implementation-defined error. */
		filemap(native_file_type fd, std::ptrdiff_t offset, std::size_t size, openmode mode = in, const char *name = nullptr)
			: handle(fd, offset, size, mode, name)
		{
		}

		/** Returns the size (in bytes) of the file mapping. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return handle.size(); }
		/** Returns pointer to the start of the mapped file. */
		[[nodiscard]] constexpr void *data() const noexcept { return handle.data(); }

		/** Returns an `std::byte` span to the mapped memory. */
		[[nodiscard]] constexpr std::span<std::byte> bytes() const noexcept
		{
			return std::span<std::byte>{static_cast<std::byte *>(data()), static_cast<std::size_t>(size())};
		}

		/** Flushes the mapped file.
		 * @param n Amount of bytes to flush, up to the size of the mapped file.
		 * @throw filemap_error On any implementation-defined error. */
		void flush(std::ptrdiff_t n) const { handle.flush(n); }

		/** Returns the underlying native handle. */
		[[nodiscard]] native_handle_type native_handle() const noexcept { return handle.native_handle(); }

	private:
		detail::filemap_handle handle;
	};
}	 // namespace sek