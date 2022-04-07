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
		SEK_API static native_handle_type handle_from_view(void *) noexcept;
		SEK_API void init(native_file_type, std::ptrdiff_t, std::ptrdiff_t, filemap_openmode, const char *);

	public:
		filemap_handle(native_file_type fd, std::ptrdiff_t offset, std::ptrdiff_t size, filemap_openmode mode, const char *name)
		{
			init(fd, offset, size, mode, name);
		}
		SEK_API filemap_handle(const wchar_t *path, std::ptrdiff_t offset, std::ptrdiff_t size, filemap_openmode mode, const char *name);
		SEK_API ~filemap_handle();

		[[nodiscard]] constexpr std::ptrdiff_t size() const noexcept { return map_size; }
		[[nodiscard]] constexpr void *data() const noexcept { return view_ptr; }

		SEK_API void flush(std::ptrdiff_t n) const;

		[[nodiscard]] native_handle_type native_handle() const noexcept { return handle_from_view(view_ptr); }

	private:
		void *view_ptr = nullptr;
		std::ptrdiff_t map_size = 0;
	};
}	 // namespace sek::detail
