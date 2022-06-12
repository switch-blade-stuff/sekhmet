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
	/** @brief Proxy type used to bind archive write operations. */
	template<typename C, typename Traits = std::char_traits<C>>
	class archive_writer
	{
	public:
		using char_type = C;
		using traits_type = Traits;
		using int_type = typename traits_type::int_type;

		struct vtable_t
		{
			std::size_t (*putn)(void *, const char_type *, std::size_t);
			std::size_t (*tell)(void *);
			void (*put)(void *, char_type c);
			void (*flush)(void *);
		};

	private:
		using sbuf_type = std::basic_streambuf<char_type, traits_type>;

		constexpr static vtable_t sbuf_vtable = {
			.putn = +[](void *data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto *sbuf = static_cast<sbuf_type *>(data);
				return static_cast<std::size_t>(sbuf->sputn(src, static_cast<std::streamsize>(n)));
			},
			.tell = +[](void *data) -> std::size_t
			{
				auto *sbuf = static_cast<sbuf_type *>(data);
				return static_cast<std::size_t>(sbuf->pubseekoff(0, std::ios::cur, std::ios::in));
			},
			.put = +[](void *data, char_type c) -> void { static_cast<sbuf_type *>(data)->sputc(c); },
			.flush = +[](void *data) -> void { static_cast<sbuf_type *>(data)->pubsync(); },
		};
		constexpr static vtable_t file_vtable = {
			.putn = +[](void *data, const char_type *src, std::size_t n) -> std::size_t
			{
				auto *file = static_cast<FILE *>(data);
				return fwrite(src, sizeof(char_type), n, file);
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
			.put = +[](void *data, char_type c) -> void
			{
				auto *file = static_cast<FILE *>(data);
				if constexpr (sizeof(char_type) == sizeof(wchar_t))
					putwc(static_cast<wchar_t>(c), file);
				else
					putc(c, file);
			},
			.flush = +[](void *data) -> void { fflush(static_cast<FILE *>(data)); },
		};

	public:
		/** Initializes an empty writer. */
		constexpr archive_writer() noexcept = default;

		/** Initializes a writer using a callback vtable and a data pointer. */
		constexpr archive_writer(const vtable_t *callback_vtable, void *data) noexcept
			: m_vtable(callback_vtable), m_data(data)
		{
		}
		/** Initializes a writer from a stream buffer. */
		constexpr archive_writer(sbuf_type *sbuf) noexcept : archive_writer(&sbuf_vtable, sbuf) {}
		/** Initializes a writer from a C file. */
		constexpr archive_writer(FILE *file) noexcept : archive_writer(&file_vtable, file) {}

		/** Checks if the writer was fully initialized. */
		[[nodiscard]] constexpr bool empty() { return m_vtable == nullptr; }

		std::size_t putn(const char_type *src, std::size_t n) { return m_vtable->putn(m_data, src, n); }
		std::size_t tell() { return m_vtable->tell(m_data); }
		void put(char_type c) { m_vtable->put(m_data, c); }
		void flush() { m_vtable->flush(m_data); }

	private:
		const vtable_t *m_vtable = nullptr;
		void *m_data;
	};
}	 // namespace sek::serialization
