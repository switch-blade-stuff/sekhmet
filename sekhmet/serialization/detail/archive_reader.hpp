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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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

		struct vtable_t
		{
			std::size_t (*getn)(void *, char_type *, std::size_t);
			std::size_t (*bump)(void *, std::size_t);
			std::size_t (*tell)(void *);
			int_type (*peek)(void *);
			int_type (*take)(void *);
		};

	private:
		using sbuf_type = std::basic_streambuf<char_type, traits_type>;

		constexpr static vtable_t sbuf_vtable = {
			.getn = +[](void *data, char_type *dst, std::size_t n) -> std::size_t
			{
				auto *sbuf = static_cast<sbuf_type *>(data);
				return static_cast<std::size_t>(sbuf->sgetn(dst, static_cast<std::streamsize>(n)));
			},
			.bump = +[](void *data, std::size_t n) -> std::size_t
			{
				auto *sbuf = static_cast<sbuf_type *>(data);
				const auto off = static_cast<std::streamoff>(n);
				if (sbuf->pubseekoff(off, std::ios::cur, std::ios::in) == typename sbuf_type::pos_type{off}) [[likely]]
					return n;
				else
					return 0;
			},
			.tell = +[](void *data) -> std::size_t
			{
				auto *sbuf = static_cast<sbuf_type *>(data);
				return static_cast<std::size_t>(sbuf->pubseekoff(0, std::ios::cur, std::ios::in));
			},
			.peek = +[](void *data) -> int_type { return static_cast<sbuf_type *>(data)->sgetc(); },
			.take = +[](void *data) -> int_type { return static_cast<sbuf_type *>(data)->sbumpc(); },
		};
		constexpr static vtable_t file_vtable = {
			.getn = +[](void *data, char_type *dst, std::size_t n) -> std::size_t
			{
				auto *file = static_cast<FILE *>(data);
				return fread(dst, sizeof(char_type), n, file);
			},
			.bump = +[](void *data, std::size_t n) -> std::size_t
			{
				auto *file = static_cast<FILE *>(data);
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
			.tell = +[](void *data) -> std::size_t
			{
				auto *file = static_cast<FILE *>(data);
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
			.peek = +[](void *data) -> int_type
			{
				auto *file = static_cast<FILE *>(data);
				if constexpr (sizeof(char_type) == sizeof(wchar_t))
					return static_cast<int_type>(ungetwc(getwc(file), file));
				else
					return static_cast<int_type>(ungetc(getc(file), file));
			},
			.take = +[](void *data) -> int_type
			{
				auto *file = static_cast<FILE *>(data);
				if constexpr (sizeof(char_type) == sizeof(wchar_t))
					return static_cast<int_type>(getwc(file));
				else
					return static_cast<int_type>(getc(file));
			},
		};

	public:
		/** Initializes an empty reader. */
		constexpr archive_reader() noexcept = default;

		/** Initializes a reader using a callback vtable and a data pointer. */
		constexpr archive_reader(const vtable_t *callback_vtable, void *data) noexcept
			: vtable(callback_vtable), data(data)
		{
		}
		/** Initializes a reader from a stream buffer. */
		constexpr archive_reader(sbuf_type *sbuf) noexcept : archive_reader(&sbuf_vtable, sbuf) {}
		/** Initializes a reader from a C file. */
		constexpr archive_reader(FILE *file) noexcept : archive_reader(&file_vtable, file) {}

		/** Checks if the reader was fully initialized. */
		[[nodiscard]] constexpr bool empty() { return vtable == nullptr; }

		std::size_t getn(char_type *dst, std::size_t n) { return vtable->getn(data, dst, n); }
		std::size_t bump(std::size_t n) { return vtable->bump(data, n); }
		std::size_t tell() { return vtable->tell(data); }
		int_type peek() { return vtable->peek(data); }
		int_type take() { return vtable->take(data); }

	private:
		const vtable_t *vtable = nullptr;
		void *data;
	};
}	 // namespace sek::serialization
