//
// Created by switchblade on 2022-04-07.
//

#pragma once

namespace sek::detail
{
	class filemap_handle
	{
	public:
		using native_handle_type = void *;
		using native_file_type = void *;

	private:
		SEK_API void init(native_file_type, std::ptrdiff_t, std::size_t, filemap_openmode, const char *);

	public:
		constexpr filemap_handle(filemap_handle &&other) noexcept
			: view_ptr(std::exchange(other.view_ptr, nullptr)), map_size(std::exchange(other.map_size, 0))
		{
		}
		constexpr filemap_handle &operator=(filemap_handle &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(filemap_handle &other) noexcept
		{
			std::swap(view_ptr, other.view_ptr);
			std::swap(map_size, other.map_size);
		}

		filemap_handle(native_file_type fd, std::ptrdiff_t offset, std::size_t size, filemap_openmode mode, const char *name)
		{
			init(fd, offset, size, mode, name);
		}
		SEK_API filemap_handle(const wchar_t *path, std::ptrdiff_t offset, std::size_t size, filemap_openmode mode, const char *name);

		[[nodiscard]] constexpr std::size_t size() const noexcept { return map_size; }
		[[nodiscard]] constexpr void *data() const noexcept { return view_ptr; }

		SEK_API bool reset() noexcept;
		SEK_API void flush(std::size_t n) const;

		[[nodiscard]] SEK_API native_handle_type native_handle() const noexcept;

	private:
		void *view_ptr = nullptr;
		std::size_t map_size = 0;
	};
}	 // namespace sek::detail
