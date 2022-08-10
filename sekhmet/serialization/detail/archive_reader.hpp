/*
 * Created by switchblade on 19/05/22
 */

#pragma once

#include <cwchar>
#include <ios>

#include "sekhmet/system/native_file.hpp"

#include "util.hpp"

namespace sek::serialization
{
	/** @brief Proxy type used to bind archive read operations. */
	template<typename C, typename Traits = std::char_traits<C>>
	class archive_reader
	{
	public:
		using char_type = C;
		using traits_type = Traits;
		using int_type = typename traits_type::int_type;

		struct callback_info
		{
			std::size_t (*getn)(void *, char_type *, std::size_t);
			std::size_t (*bump)(void *, std::size_t);
			std::size_t (*tell)(void *);
			int_type (*peek)(void *);
			int_type (*take)(void *);
		};

	private:
		using sbuf_type = std::basic_streambuf<char_type, traits_type>;

		struct callback_data_t
		{
			constexpr callback_data_t(const callback_info *callbacks, void *user_data) noexcept
				: callbacks(callbacks), user_data(user_data)
			{
			}

			const callback_info *callbacks;
			void *user_data;
		};
		struct buffer_data_t
		{
			constexpr buffer_data_t(const char_type *data, std::size_t size) noexcept : data(data), size(size) {}

			const char_type *data;
			std::size_t size;
			std::size_t pos = 0;
		};
		union data_t
		{
			constexpr data_t() noexcept : padding() {}

			constexpr data_t(const callback_info *callbacks, void *user_data) noexcept : callback(callbacks, user_data)
			{
			}
			constexpr data_t(const char_type *data, std::size_t size) noexcept : buffer(data, size) {}
			constexpr data_t(system::native_file *file) noexcept : file(file) {}
			constexpr data_t(sbuf_type *stream_buf) noexcept : stream_buf(stream_buf) {}
			constexpr data_t(FILE *file) noexcept : c_file(file) {}

			constexpr void swap(data_t &other) noexcept { std::swap(padding, other.padding); }

			std::byte padding[sizeof(void *) * 3];

			system::native_file *file;
			callback_data_t callback;
			sbuf_type *stream_buf;
			buffer_data_t buffer;
			FILE *c_file;
		};

		struct vtable_t
		{
			std::size_t (*getn)(data_t &, char_type *, std::size_t);
			std::size_t (*bump)(data_t &, std::size_t);
			std::size_t (*tell)(data_t &);
			int_type (*peek)(data_t &);
			int_type (*take)(data_t &);
		};

		constexpr static vtable_t callback_vtable = {
			.getn = +[](data_t &data, char_type *dst, std::size_t n) -> std::size_t
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				return callbacks->getn(user_data, dst, n);
			},
			.bump = +[](data_t &data, std::size_t n) -> std::size_t
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				return callbacks->bump(user_data, n);
			},
			.tell = +[](data_t &data) -> std::size_t
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				return callbacks->tell(user_data);
			},
			.peek = +[](data_t &data) -> int_type
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				return callbacks->peek(user_data);
			},
			.take = +[](data_t &data) -> int_type
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				return callbacks->take(user_data);
			},
		};
		constexpr static vtable_t buffer_vtable = {
			.getn = +[](data_t &data, char_type *dst, std::size_t n) -> std::size_t
			{
				auto &buffer = data.buffer;
				auto new_pos = buffer.pos + n;
				if (new_pos > buffer.size) [[unlikely]]
					new_pos = buffer.size;

				std::copy(buffer.data + buffer.pos, buffer.data + new_pos, dst);
				return new_pos - std::exchange(buffer.pos, new_pos);
			},
			.bump = +[](data_t &data, std::size_t n) -> std::size_t
			{
				auto &buffer = data.buffer;
				auto new_pos = buffer.pos + n;
				if (new_pos > buffer.size) [[unlikely]]
					new_pos = buffer.size;
				return new_pos - std::exchange(buffer.pos, new_pos);
			},
			.tell = +[](data_t &data) -> std::size_t { return data.buffer.pos; },
			.peek = +[](data_t &data) -> int_type
			{
				auto &buffer = data.buffer;
				if (buffer.pos < buffer.size)
					return traits_type::to_int_type(buffer.data[buffer.pos]);
				else
					return traits_type::eof();
			},
			.take = +[](data_t &data) -> int_type
			{
				auto &buffer = data.buffer;
				if (buffer.pos < buffer.size)
					return traits_type::to_int_type(buffer.data[buffer.pos++]);
				else
					return traits_type::eof();
			},
		};
		constexpr static vtable_t sbuf_vtable = {
			.getn = +[](data_t &data, char_type *dst, std::size_t n) -> std::size_t
			{
				auto *sbuf = data.stream_buf;
				return static_cast<std::size_t>(sbuf->sgetn(dst, static_cast<std::streamsize>(n)));
			},
			.bump = +[](data_t &data, std::size_t n) -> std::size_t
			{
				const auto pos = data.stream_buf->pubseekoff(0, std::ios::cur, std::ios::in);
				const auto new_pos = pos + +static_cast<std::streamoff>(n);
				if (data.stream_buf->pubseekpos(new_pos, std::ios::in) == new_pos) [[likely]]
					return n;
				else
					return 0;
			},
			.tell = +[](data_t &data) -> std::size_t
			{
				auto *sbuf = data.stream_buf;
				return static_cast<std::size_t>(sbuf->pubseekoff(0, std::ios::cur, std::ios::in));
			},
			.peek = +[](data_t &data) -> int_type { return data.stream_buf->sgetc(); },
			.take = +[](data_t &data) -> int_type { return data.stream_buf->sbumpc(); },
		};
		constexpr static vtable_t native_file_vtable = {
			.getn = +[](data_t &data, char_type *dst, std::size_t n) { return data.file->read(dst, n * sizeof(C)); },
			.bump = +[](data_t &data, std::size_t n) -> std::size_t
			{
				const auto pos = data.file->tell() + static_cast<std::int64_t>(n);
				if (data.file->seek(pos, system::native_file::seek_set) == pos) [[likely]]
					return n;
				else
					return 0;
			},
			.tell = +[](data_t &data) { return static_cast<std::size_t>(data.file->tell()); },
			.peek = +[](data_t &data) -> int_type
			{
				char_type c;
				if (data.file->read(&c, sizeof(char_type)) == sizeof(char_type)) [[likely]]
				{
					data.file->seek(-static_cast<std::int64_t>(sizeof(char_type)), system::native_file::seek_set);
					return traits_type::to_int_type(c);
				}
				else
					return traits_type::eof();
			},
			.take = +[](data_t &data) -> int_type
			{
				char_type c;
				if (data.file->read(&c, sizeof(char_type)) == sizeof(char_type)) [[likely]]
					return traits_type::to_int_type(c);
				else
					return traits_type::eof();
			},
		};
		constexpr static vtable_t c_file_vtable = {
			.getn = +[](data_t &data, char_type *dst, std::size_t n) { return fread(dst, sizeof(C), n, data.c_file); },
			.bump = +[](data_t &data, std::size_t n) -> std::size_t
			{
				auto *file = data.c_file;
				const auto off = sizeof(C) * n;
#if defined(_POSIX_C_SOURCE)
#if _FILE_OFFSET_BITS < 64
				auto err = fseeko64(file, static_cast<off64_t>(off), SEEK_CUR);
#else
				auto err = fseeko(file, static_cast<off_t>(off), SEEK_CUR);
#endif
#elif defined(SEK_OS_WIN)
				auto err = _fseeki64(file, static_cast<__int64>(off), SEEK_CUR);
#else
				auto err = fseek(file, static_cast<long>(off), SEEK_CUR);
#endif
				if (!err) [[likely]]
					return n;
				else
					return 0;
			},
			.tell = +[](data_t &data) -> std::size_t
			{
				auto *file = data.c_file;
#if defined(_POSIX_C_SOURCE)
#if _FILE_OFFSET_BITS < 64
				auto pos = ftello64(file);
#else
				auto pos = ftello(file);
#endif
#elif defined(SEK_OS_WIN)
				auto pos = _ftelli64(file);
#else
				auto pos = ftell(file);
#endif
				if (pos >= 0) [[likely]]
					return static_cast<std::size_t>(pos) / sizeof(C);
				else
					return static_cast<std::size_t>(EOF);
			},
			.peek = +[](data_t &data) -> int_type
			{
				auto *file = data.c_file;
				if constexpr (sizeof(C) == sizeof(wchar_t))
					return static_cast<int_type>(ungetwc(getwc(file), file));
				else
					return static_cast<int_type>(ungetc(getc(file), file));
			},
			.take = +[](data_t &data) -> int_type
			{
				auto *file = data.c_file;
				if constexpr (sizeof(C) == sizeof(wchar_t))
					return static_cast<int_type>(getwc(file));
				else
					return static_cast<int_type>(getc(file));
			},
		};

		constexpr archive_reader(const vtable_t *vtable, data_t data) noexcept : m_vtable(vtable), m_data(data) {}

	public:
		/** Initializes an empty reader. */
		constexpr archive_reader() noexcept = default;
		constexpr archive_reader(archive_reader &&other) noexcept { swap(other); }
		constexpr archive_reader &operator=(archive_reader &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Initializes a reader using user-provided callbacks and data. */
		constexpr archive_reader(const callback_info *callbacks, void *data) noexcept
			: archive_reader(&callback_vtable, {callbacks, data})
		{
		}
		/** Initializes a reader from a data buffer. */
		constexpr archive_reader(const char_type *data, std::size_t n) noexcept
			: archive_reader(&buffer_vtable, {data, n})
		{
		}
		/** @copydoc archive_reader */
		constexpr archive_reader(const void *data, std::size_t n) noexcept
			: archive_reader(static_cast<const C *>(data), n / sizeof(C))
		{
		}
		/** Initializes a reader from a native file. */
		constexpr archive_reader(system::native_file &file) noexcept : archive_reader(&native_file_vtable, {&file}) {}
		/** Initializes a reader from a stream buffer. */
		constexpr archive_reader(sbuf_type *sbuf) noexcept : archive_reader(&sbuf_vtable, {sbuf}) {}
		/** Initializes a reader from a C file. */
		constexpr archive_reader(FILE *file) noexcept : archive_reader(&c_file_vtable, {file}) {}

		/** Checks if the reader was fully initialized. */
		[[nodiscard]] constexpr bool empty() { return m_vtable == nullptr; }

		std::size_t getn(char_type *dst, std::size_t n) { return m_vtable->getn(m_data, dst, n); }
		std::size_t bump(std::size_t n) { return m_vtable->bump(m_data, n); }
		std::size_t tell() { return m_vtable->tell(m_data); }
		int_type peek() { return m_vtable->peek(m_data); }
		int_type take() { return m_vtable->take(m_data); }

		constexpr void swap(archive_reader &other) noexcept
		{
			std::swap(m_vtable, other.m_vtable);
			m_data.swap(other.m_data);
		}
		friend constexpr void swap(archive_reader &a, archive_reader &b) noexcept { a.swap(b); }

	private:
		const vtable_t *m_vtable = nullptr;
		data_t m_data;
	};
}	 // namespace sek::serialization
