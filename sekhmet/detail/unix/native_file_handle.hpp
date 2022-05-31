//
// Created by switchblade on 30/05/22.
//

#pragma once

namespace sek::detail
{
	class native_file_handle
	{
	public:
		using native_type = int;

	public:
		constexpr native_file_handle() noexcept = default;
		constexpr native_file_handle(native_file_handle &&other) noexcept { swap(other); }
		constexpr native_file_handle &operator=(native_file_handle &&other) noexcept
		{
			swap(other);
			return *this;
		}

		SEK_API bool open(const char *path, native_openmode mode);
		SEK_API bool close();

		SEK_API std::size_t write(const void *src, std::size_t n);
		SEK_API std::size_t read(void *dst, std::size_t n);
		SEK_API ssize_t seek(ssize_t pos, int way);
		SEK_API bool sync();

		[[nodiscard]] constexpr bool is_open() const noexcept { return fd >= 0; }
		[[nodiscard]] constexpr native_type native_handle() const noexcept { return fd; }

		constexpr void swap(native_file_handle &other) noexcept { std::swap(fd, other.fd); }

	private:
		native_type fd = -1;
	};
}	 // namespace sek::detail