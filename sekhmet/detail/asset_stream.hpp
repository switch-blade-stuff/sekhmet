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
 * Created by switchblade on 04/05/22
 */

#pragma once

#include <bit>
#include <fstream>
#include <sstream>

#include "define.h"

namespace sek
{
	/** @brief Stream used to read & write assets either using a file or a memory buffer. */
	template<typename C, typename Traits = std::char_traits<C>, typename Alloc = std::allocator<C>>
	class basic_asset_stream : std::basic_iostream<C, Traits>
	{
		using base_t = std::basic_iostream<C, Traits>;

	public:
		typedef C char_type;
		typedef Traits traits_type;
		typedef typename traits_type::int_type int_type;
		typedef typename traits_type::pos_type pos_type;
		typedef typename traits_type::off_type off_type;

		typedef std::basic_filebuf<C, Traits> filebuf_type;
		typedef std::basic_stringbuf<C, Traits, Alloc> stringbuf_type;

	private:
		enum mode_t : int
		{
			FILEBUF = 1,
			STRINGBUF = 2,
		};

		typedef std::basic_streambuf<C, Traits> streambuf_type;
		constexpr static auto fb_size = sizeof(filebuf_type);
		constexpr static auto sb_size = sizeof(stringbuf_type);
		constexpr static auto pad_size = fb_size > sb_size ? fb_size : sb_size;

	public:
		basic_asset_stream() = delete;
		basic_asset_stream(const basic_asset_stream &) = delete;
		basic_asset_stream &operator=(const basic_asset_stream &) = delete;

		// clang-format off
		basic_asset_stream(basic_asset_stream &&other)
			noexcept(std::is_nothrow_move_constructible_v<filebuf_type> &&
			         std::is_nothrow_move_constructible_v<stringbuf_type>)
			: mode(other.mode)
		{
			if (mode == FILEBUF)
			{
				std::construct_at(&fb, std::move(other.fb));
				base_t::rdbuf(&fb);
			}
			else
			{
				std::construct_at(&sb, std::move(other.sb));
				base_t::rdbuf(&sb);
			}
		}
		basic_asset_stream &operator=(basic_asset_stream &&other)
		{
			if (mode != other.mode)
			{
				if (mode == FILEBUF)
				{
					tmp_move(fb, sb, other.sb);
					base_t::rdbuf(&fb);
				}
				else
				{
					tmp_move(sb, fb, other.fb);
					base_t::rdbuf(&sb);
				}
			}
			else if (mode == FILEBUF)
				fb = std::move(other.fb);
			else
				sb = std::move(other.sb);
			return *this;
		}
		// clang-format on

		/** Initializes asset stream from a file buffer. */
		explicit basic_asset_stream(filebuf_type &&fb) noexcept(std::is_nothrow_move_constructible_v<filebuf_type>)
			: base_t(std::bit_cast<streambuf_type *>(&pad)), fb(std::move(fb)), mode(FILEBUF)
		{
		}
		/** Initializes asset stream from a string buffer. */
		explicit basic_asset_stream(stringbuf_type &&sb) noexcept(std::is_nothrow_move_constructible_v<stringbuf_type>)
			: base_t(std::bit_cast<streambuf_type *>(&pad)), sb(std::move(sb)), mode(STRINGBUF)
		{
		}
		/** Initializes asset stream from a string. */
		template<typename A>
		explicit basic_asset_stream(const std::basic_string<C, Traits, A> &str,
									std::ios::openmode mode = std::ios::in | std::ios::out)
			: base_t(std::bit_cast<streambuf_type *>(&pad)), sb(str, mode), mode(STRINGBUF)
		{
		}
		/** @copydoc basic_asset_stream */
		explicit basic_asset_stream(std::basic_string<C, Traits, Alloc> &&str,
									std::ios::openmode mode = std::ios::in | std::ios::out)
			: base_t(std::bit_cast<streambuf_type *>(&pad)), sb(std::move(str), mode), mode(STRINGBUF)
		{
		}

		~basic_asset_stream() override
		{
			switch (mode)
			{
				case FILEBUF: std::destroy_at(&fb); break;
				case STRINGBUF: std::destroy_at(&sb); break;
			}
		}

		constexpr void swap(basic_asset_stream &other) noexcept
		{
			if (mode != other.mode) [[unlikely]]
			{
				if (mode == FILEBUF)
				{
					filebuf_type fb_tmp = std::move(fb);
					tmp_move(fb, sb, other.sb);
					std::construct_at(&other.fb, std::move(fb_tmp));
				}
				else
				{
					stringbuf_type sb_tmp = std::move(sb);
					tmp_move(sb, fb, other.fb);
					std::construct_at(&other.sb, std::move(sb_tmp));
				}
			}
			else if (mode == FILEBUF)
				std::swap(fb, other.fb);
			else
				std::swap(sb, other.sb);
		}
		friend constexpr void swap(basic_asset_stream &a, basic_asset_stream &b) noexcept { a.swap(b); }

	private:
		template<typename Old, typename To, typename From>
		constexpr void tmp_move(Old &old, To &to, From &from)
		{
			std::destroy_at(&old);
			std::construct_at(&to, std::move(from));
			std::destroy_at(&from);
		}

		union
		{
			std::byte pad[pad_size] = {};
			filebuf_type fb;
			stringbuf_type sb;
		};
		mode_t mode;
	};

	using asset_stream = basic_asset_stream<char>;
	using wasset_stream = basic_asset_stream<wchar_t>;

	extern template class SEK_API_IMPORT basic_asset_stream<char>;
	extern template class SEK_API_IMPORT basic_asset_stream<wchar_t>;
}	 // namespace sek