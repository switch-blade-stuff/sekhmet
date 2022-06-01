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

		struct generic_data_t
		{
			const callback_info *callbacks;
			void *data_ptr;
		};
		struct buffer_data_t
		{
			const char_type *begin;
			const char_type *curr;
			const char_type *end;
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
			std::size_t (*getn)(data_t *, char_type *, std::size_t);
			std::size_t (*bump)(data_t *, std::size_t);
			std::size_t (*tell)(data_t *);
			int_type (*peek)(data_t *);
			int_type (*take)(data_t *);
		};

		constexpr static vtable_t generic_vtable = {
			.getn = +[](data_t *data, char_type *dst, std::size_t n) -> std::size_t
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				return callbacks.getn(data_ptr, dst, n);
			},
			.bump = +[](data_t *data, std::size_t n) -> std::size_t
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				return callbacks.bump(data_ptr, n);
			},
			.tell = +[](data_t *data) -> std::size_t
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				return callbacks.tell(data_ptr);
			},
			.peek = +[](data_t *data) -> int_type
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				return callbacks.peek(data_ptr);
			},
			.take = +[](data_t *data) -> int_type
			{
				const auto callbacks = data->generic.callbacks;
				const auto data_ptr = data->generic.data_ptr;
				return callbacks.take(data_ptr);
			},
		};
		constexpr static vtable_t membuf_vtable = {
			.getn = +[](data_t *data, char_type *dst, std::size_t n) -> std::size_t
			{
				auto &membuf = data->membuf;
				auto new_curr = membuf.curr + n;
				if (new_curr >= membuf.end) [[unlikely]]
					new_curr = membuf.end;
				std::copy(membuf.curr, new_curr, dst);
				return static_cast<std::size_t>(new_curr - std::exchange(membuf.curr, new_curr));
			},
			.bump = +[](data_t *data, std::size_t n) -> std::size_t
			{
				auto &membuf = data->membuf;
				auto new_curr = membuf.curr + n;
				if (new_curr >= membuf.end) [[unlikely]]
					new_curr = membuf.end;
				return static_cast<std::size_t>(new_curr - std::exchange(membuf.curr, new_curr));
			},
			.tell = +[](data_t *data) -> std::size_t
			{
				auto &membuf = data->membuf;
				return static_cast<std::size_t>(membuf.curr - membuf.begin);
			},
			.peek = +[](data_t *data) -> int_type
			{
				auto &membuf = data->membuf;
				return membuf.end > membuf.curr ? traits_type::to_int_type(*membuf.curr) : traits_type::eof();
			},
			.take = +[](data_t *data) -> int_type
			{
				auto &membuf = data->membuf;
				return membuf.end > membuf.curr ? traits_type::to_int_type(*membuf.curr++) : traits_type::eof();
			},
		};
		constexpr static vtable_t sbuf_vtable = {
			.getn = +[](data_t *data, char_type *dst, std::size_t n) -> std::size_t
			{
				const auto sbuf = data->sbuf;
				return static_cast<std::size_t>(sbuf->sgetn(dst, static_cast<std::streamsize>(n)));
			},
			.bump = +[](data_t *data, std::size_t n) -> std::size_t
			{
				const auto sbuf = data->sbuf;
				const auto off = static_cast<std::streamoff>(n);
				if (sbuf->pubseekoff(off, std::ios::cur, std::ios::in) == typename sbuf_type::pos_type{off}) [[likely]]
					return n;
				else
					return 0;
			},
			.tell = +[](data_t *data) -> std::size_t
			{
				const auto sbuf = data->sbuf;
				return static_cast<std::size_t>(sbuf->pubseekoff(0, std::ios::cur, std::ios::in));
			},
			.peek = +[](data_t *data) -> int_type { return data->sbuf->sgetc(); },
			.take = +[](data_t *data) -> int_type { return data->sbuf->sbumpc(); },
		};
		constexpr static vtable_t file_vtable = {
			.getn = +[](data_t *data, char_type *dst, std::size_t n) -> std::size_t
			{
				const auto file = data->file;
				return fread(dst, sizeof(char_type), n, file);
			},
			.bump = +[](data_t *data, std::size_t n) -> std::size_t
			{
				const auto file = data->file;
				const auto off = sizeof(char_type) * n;
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
			.peek = +[](data_t *data) -> int_type
			{
				const auto file = data->file;
				if constexpr (sizeof(char_type) == sizeof(wchar_t))
					return static_cast<int_type>(ungetwc(getwc(file), file));
				else
					return static_cast<int_type>(ungetc(getc(file), file));
			},
			.take = +[](data_t *data) -> int_type
			{
				const auto file = data->file;
				if constexpr (sizeof(char_type) == sizeof(wchar_t))
					return static_cast<int_type>(getwc(file));
				else
					return static_cast<int_type>(getc(file));
			},
		};

	public:
		archive_reader() = delete;

		constexpr archive_reader(const callback_info *callbacks, void *data) noexcept
			: vtable(&generic_vtable), data(generic_data_t{callbacks, data})
		{
		}
		constexpr archive_reader(const char_type *first, const char_type *last) noexcept
			: vtable(&membuf_vtable), data(buffer_data_t{first, first, last})
		{
		}
		constexpr archive_reader(const char_type *first, std::size_t n) noexcept : archive_reader(first, first + n) {}
		constexpr archive_reader(std::basic_string_view<C, Traits> sv) noexcept : archive_reader(sv.data(), sv.size())
		{
		}
		template<std::size_t N>
		constexpr archive_reader(const std::array<C, N> &data) noexcept : archive_reader(data.data(), N)
		{
		}
		template<std::size_t N>
		constexpr archive_reader(const C (&chars)[N]) noexcept : archive_reader(chars, N)
		{
		}
		constexpr archive_reader(sbuf_type *sbuf) noexcept : vtable(&sbuf_vtable), data(sbuf) {}
		constexpr archive_reader(FILE *file) noexcept : vtable(&file_vtable), data(file) {}

		std::size_t getn(char_type *dst, std::size_t n) { return vtable->getn(&data, dst, n); }
		std::size_t bump(std::size_t n) { return vtable->bump(&data, n); }
		std::size_t tell() { return vtable->tell(&data); }
		int_type peek() { return vtable->peek(&data); }
		int_type take() { return vtable->take(&data); }

	private:
		const vtable_t *vtable;
		data_t data;
	};
}	 // namespace sek::serialization
