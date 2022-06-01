/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 19/05/22
 */

#pragma once

#include <cwchar>
#include <ios>

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

		struct generic_data_t
		{
			const callback_info *callbacks;
			void *data_ptr;
		};
		struct buffer_data_t
		{
			char_type *begin;
			char_type *curr;
			char_type *end;
		};
		union data_t
		{
			constexpr data_t(generic_data_t *generic) noexcept : generic(generic) {}
			constexpr data_t(buffer_data_t membuf) noexcept : membuf(membuf) {}
			constexpr data_t(sbuf_type *sbuf) noexcept : sbuf(sbuf) {}
			constexpr data_t(FILE *file) noexcept : file(file) {}

			generic_data_t generic;
			buffer_data_t membuf;
			sbuf_type *sbuf;
			FILE *file;
		};

		struct vtable_t
		{
			std::size_t (*putn)(data_t *, const char_type *, std::size_t);
			std::size_t (*tell)(data_t *);
			void (*put)(data_t *, char_type c);
			void (*flush)(data_t *);
		};

		constexpr static vtable_t generic_vtable = {
			.putn = +[](data_t *data, const char_type *src, std::size_t n) -> std::size_t
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				return callbacks->putn(data_ptr, src, n);
			},
			.tell = +[](data_t *data) -> std::size_t
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				return callbacks->tell(data_ptr);
			},
			.put = +[](data_t *data, char_type c) -> void
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				callbacks->put(data_ptr, c);
			},
			.flush = +[](data_t *data) -> void
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				callbacks->flush(data_ptr);
			},
		};
		constexpr static vtable_t membuf_vtable = {
			.putn = +[](data_t *data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto &membuf = data->membuf;
				if (membuf.curr + n > membuf.end) [[unlikely]]
					n = static_cast<std::size_t>(membuf.end - membuf.curr);
				membuf.curr = std::copy_n(src, n, membuf.curr);
				return n;
			},
			.tell = +[](data_t *data) -> std::size_t
			{
				auto &membuf = data->membuf;
				return static_cast<std::size_t>(membuf.curr - membuf.begin);
			},
			.put = +[](data_t *data, char_type c) -> void
			{
				auto &membuf = data->membuf;
				if (membuf.curr < membuf.end) [[likely]]
					*membuf.curr++ = c;
			},
			.flush = +[](data_t *) -> void {},
		};
		constexpr static vtable_t sbuf_vtable = {
			.putn = +[](data_t *data, const char_type *src, std::size_t n) -> std::size_t
			{
				const auto sbuf = data->sbuf;
				return static_cast<std::size_t>(sbuf->sputn(src, static_cast<std::streamsize>(n)));
			},
			.tell = +[](data_t *data) -> std::size_t
			{
				const auto sbuf = data->sbuf;
				return static_cast<std::size_t>(sbuf->pubseekoff(0, std::ios::cur, std::ios::in));
			},
			.put = +[](data_t *data, char_type c) -> void { data->sbuf->sputc(c); },
			.flush = +[](data_t *data) -> void { data->sbuf->pubsync(); },
		};
		constexpr static vtable_t file_vtable = {
			.putn = +[](data_t *data, const char_type *src, std::size_t n) -> std::size_t
			{
				const auto file = data->file;
				return fwrite(src, sizeof(char_type), n, file);
			},
			.tell = +[](data_t *data) -> std::size_t
			{
				const auto file = data->file;
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
			.put = +[](data_t *data, char_type c) -> void
			{
				const auto file = data->file;
				if constexpr (sizeof(char_type) == sizeof(wchar_t))
					putwc(static_cast<wchar_t>(c), file);
				else
					putc(c, file);
			},
			.flush = +[](data_t *data) -> void { fflush(data->file); },
		};

	public:
		archive_writer() = delete;

		constexpr archive_writer(const callback_info *callbacks, void *data) noexcept
			: vtable(&generic_vtable), data(generic_data_t{callbacks, data})
		{
		}
		constexpr archive_writer(char_type *first, char_type *last) noexcept
			: vtable(&membuf_vtable), data(buffer_data_t{first, first, last})
		{
		}
		constexpr archive_writer(char_type *first, std::size_t n) noexcept : archive_writer(first, first + n) {}
		constexpr archive_writer(std::basic_string_view<C, Traits> sv) noexcept : archive_writer(sv.data(), sv.size())
		{
		}
		template<std::size_t N>
		constexpr archive_writer(std::array<C, N> &data) noexcept : archive_writer(data.data(), N)
		{
		}
		template<std::size_t N>
		constexpr archive_writer(C (&chars)[N]) noexcept : archive_writer(chars, N)
		{
		}
		constexpr archive_writer(sbuf_type *sbuf) noexcept : vtable(&sbuf_vtable), data(sbuf) {}
		constexpr archive_writer(FILE *file) noexcept : vtable(&file_vtable), data(file) {}

		std::size_t putn(const char_type *src, std::size_t n) { return vtable->putn(&data, src, n); }
		std::size_t tell() { return vtable->tell(&data); }
		void put(char_type c) { vtable->put(&data, c); }
		void flush() { vtable->flush(&data); }

	private:
		const vtable_t *vtable;
		data_t data;
	};
}	 // namespace sek::serialization
