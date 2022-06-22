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
	/** @brief Proxy type used to bind archive write operations. */
	template<typename C, typename Traits = std::char_traits<C>>
	class archive_writer
	{
	public:
		using char_type = C;
		using traits_type = Traits;
		using int_type = typename traits_type::int_type;

		struct callback_info
		{
			std::size_t (*putn)(void *, const char_type *, std::size_t);
			std::size_t (*tell)(void *);
			void (*put)(void *, char_type c);
			void (*flush)(void *);
		};

	private:
		using sbuf_type = std::basic_streambuf<char_type, traits_type>;
		template<typename A>
		using string_type = std::basic_string<char_type, traits_type, A>;

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
			constexpr buffer_data_t(char_type *data, std::size_t size) noexcept : data(data), size(size) {}

			char_type *data;
			std::size_t size;
			std::size_t pos = 0;
		};
		union data_t
		{
			constexpr data_t() noexcept : padding() {}

			constexpr data_t(const callback_info *callbacks, void *user_data) noexcept : callback(callbacks, user_data)
			{
			}
			constexpr data_t(char_type *data, std::size_t size) noexcept : buffer(data, size) {}
			constexpr data_t(system::native_file *file) noexcept : file(file) {}
			constexpr data_t(sbuf_type *stream_buf) noexcept : stream_buf(stream_buf) {}
			constexpr data_t(FILE *file) noexcept : c_file(file) {}
			template<typename A>
			constexpr data_t(string_type<A> *str) noexcept : str_ptr(str)
			{
			}

			constexpr void swap(data_t &other) noexcept { std::swap(padding, other.padding); }

			std::byte padding[sizeof(void *) * 3];

			system::native_file *file;
			callback_data_t callback;
			sbuf_type *stream_buf;
			buffer_data_t buffer;
			void *str_ptr;
			FILE *c_file;
		};

		struct vtable_t
		{
			std::size_t (*putn)(data_t &, const char_type *, std::size_t);
			std::size_t (*tell)(data_t &);
			void (*put)(data_t &, char_type c);
			void (*flush)(data_t &);
		};

		constexpr static vtable_t callback_vtable = {
			.putn = +[](data_t &data, const char_type *src, std::size_t n) -> std::size_t
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				return callbacks->putn(user_data, src, n);
			},
			.tell = +[](data_t &data) -> std::size_t
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				return callbacks->tell(user_data);
			},
			.put = +[](data_t &data, char_type c) -> void
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				callbacks->put(user_data, c);
			},
			.flush = +[](data_t &data) -> void
			{
				const auto callbacks = data.callback.callbacks;
				const auto user_data = data.callback.user_data;
				callbacks->flush(user_data);
			},
		};
		constexpr static vtable_t buffer_vtable = {
			.putn = +[](data_t &data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto &buffer = data.buffer;
				auto new_pos = buffer.pos + n;
				if (new_pos > buffer.size) [[unlikely]]
					new_pos = buffer.size;

				std::copy(src, buffer.data + buffer.pos, buffer.data + new_pos);
				return new_pos - std::exchange(buffer.pos, new_pos);
			},
			.tell = +[](data_t &data) -> std::size_t { return data.buffer.pos; },
			.put = +[](data_t &data, char_type c) -> void
			{
				if (auto &buffer = data.buffer.pos; buffer.pos != buffer.size) [[likely]]
					buffer.data[buffer.pos++] = c;
			},
			.flush = +[](data_t &) {},
		};
		template<typename A>
		constexpr static vtable_t string_vtable = {
			.putn = +[](data_t &data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto *str = static_cast<string_type<A> *>(data.str_ptr);
				str->insert(str->size(), src, n);
				return n;
			},
			.tell = +[](data_t &data) { return static_cast<string_type<A> *>(data.str_ptr)->size(); },
			.put = +[](data_t &data, char_type c) { static_cast<string_type<A> *>(data.str_ptr)->append(1, c); },
			.flush = +[](data_t &) {},
		};
		constexpr static vtable_t sbuf_vtable = {
			.putn = +[](data_t &data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto *sbuf = data.stream_buf;
				return static_cast<std::size_t>(sbuf->sputn(src, static_cast<std::streamsize>(n)));
			},
			.tell = +[](data_t &data) -> std::size_t
			{
				auto *sbuf = data.stream_buf;
				return static_cast<std::size_t>(sbuf->pubseekoff(0, std::ios::cur, std::ios::in));
			},
			.put = +[](data_t &data, char_type c) -> void { data.stream_buf->sputc(c); },
			.flush = +[](data_t &data) { data.stream_buf->pubsync(); },
		};
		constexpr static vtable_t native_file_vtable = {
			.putn = +[](data_t &data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto *file = data.file;
				return file->write(src, n * sizeof(char_type));
			},
			.tell = +[](data_t &data) { return static_cast<std::size_t>(data.file->tell()); },
			.put = +[](data_t &data, char_type c) { data.file->write(&c, sizeof(char_type)); },
			.flush = +[](data_t &data) { data.file->flush(); },
		};
		constexpr static vtable_t c_file_vtable = {
			.putn = +[](data_t &data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto *file = data.c_file;
				return fwrite(src, sizeof(char_type), n, file);
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
					return static_cast<std::size_t>(pos) / sizeof(char_type);
				else
					return static_cast<std::size_t>(EOF);
			},
			.put = +[](data_t &data, char_type c) -> void
			{
				auto *file = data.c_file;
				if constexpr (sizeof(char_type) == sizeof(wchar_t))
					putwc(static_cast<wchar_t>(c), file);
				else
					putc(c, file);
			},
			.flush = +[](data_t &data) { fflush(data.c_file); },
		};

		constexpr archive_writer(const vtable_t *vtable, data_t data) noexcept : m_vtable(vtable), m_data(data) {}

	public:
		/** Initializes an empty writer. */
		constexpr archive_writer() noexcept = default;
		constexpr archive_writer(archive_writer &&other) noexcept { swap(other); }
		constexpr archive_writer &operator=(archive_writer &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Initializes a writer using user-provided callbacks and data. */
		constexpr archive_writer(const callback_info *callbacks, void *data) noexcept
			: archive_writer(&callback_vtable, {callbacks, data})
		{
		}
		/** Initializes a writer from a data buffer. */
		constexpr archive_writer(char_type *data, std::size_t n) noexcept : archive_writer(&buffer_vtable, {data, n}) {}
		/** @copydoc archive_writer */
		constexpr archive_writer(void *data, std::size_t n) noexcept
			: archive_writer(static_cast<const C *>(data), n / sizeof(C))
		{
		}
		/** Initializes a writer from a native file. */
		constexpr archive_writer(system::native_file &file) noexcept : archive_writer(&native_file_vtable, {&file}) {}
		/** Initializes a writer from a string. */
		template<typename A = std::allocator<char_type>>
		constexpr archive_writer(string_type<A> &str) noexcept : archive_writer(&string_vtable<A>, {&str})
		{
		}
		/** Initializes a writer from a stream buffer. */
		constexpr archive_writer(sbuf_type *sbuf) noexcept : archive_writer(&sbuf_vtable, {sbuf}) {}
		/** Initializes a writer from a C file. */
		constexpr archive_writer(FILE *file) noexcept : archive_writer(&c_file_vtable, {file}) {}

		/** Checks if the writer was fully initialized. */
		[[nodiscard]] constexpr bool empty() { return m_vtable == nullptr; }

		std::size_t putn(const char_type *src, std::size_t n) { return m_vtable->putn(m_data, src, n); }
		std::size_t tell() { return m_vtable->tell(m_data); }
		void put(char_type c) { m_vtable->put(m_data, c); }
		void flush() { m_vtable->flush(m_data); }

		constexpr void swap(archive_writer &other) noexcept
		{
			std::swap(m_vtable, other.m_vtable);
			m_data.swap(other.m_data);
		}
		friend constexpr void swap(archive_writer &a, archive_writer &b) noexcept { a.swap(b); }

	private:
		const vtable_t *m_vtable = nullptr;
		data_t m_data;
	};
}	 // namespace sek::serialization
