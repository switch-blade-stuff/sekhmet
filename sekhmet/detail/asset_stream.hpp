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
 * Created by switchblade on 04/05/22
 */

#pragma once

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <ios>
#include <utility>

#include "native_file.hpp"

namespace sek
{
	/** @brief Memory buffer used by the `basic_asset_buff` stream buffer. */
	class asset_membuf
	{
		/* Buffer sizes are multiples of 4kb. */
		constexpr static std::size_t size_mult = SEK_KB(4);

	public:
		asset_membuf(const asset_membuf &) = delete;
		asset_membuf &operator=(const asset_membuf &) = delete;

		constexpr asset_membuf() noexcept = default;
		constexpr asset_membuf(asset_membuf &&other) noexcept { swap(other); }
		constexpr asset_membuf &operator=(asset_membuf &&other) noexcept
		{
			swap(other);
			return *this;
		}

		explicit asset_membuf(std::size_t n)
		{
			data = static_cast<char *>(malloc(size = n));
			if (data == nullptr) [[unlikely]]
				throw std::bad_alloc();
		}
		~asset_membuf() { free(data); }

		void resize(std::size_t new_size)
		{
			if (new_size > size)
			{
				const auto rem = new_size % size_mult;
				new_size = new_size - rem + (rem ? size_mult : 0);

				auto new_data = static_cast<char *>(realloc(data, new_size));
				if (new_data == nullptr) [[unlikely]]
					throw std::bad_alloc();
				data = new_data;
				size = new_size;
			}
		}
		[[nodiscard]] std::size_t write(const void *src, std::size_t n)
		{
			/* Re-allocate the buffer if needed. */
			const auto new_curr = curr + n;
			resize(new_curr);
			std::copy_n(static_cast<const char *>(src), n, data + std::exchange(curr, new_curr));
			return n;
		}
		[[nodiscard]] std::size_t read(void *dst, std::size_t n)
		{
			if (curr == size) [[unlikely]]
				return 0;
			else if (const auto left = size - curr; left < n) [[unlikely]]
				n = left;
			std::copy_n(data + curr, n, static_cast<char *>(dst));
			curr += n;
			return n;
		}
		void reset() noexcept { curr = 0; }

		constexpr void swap(asset_membuf &other) noexcept
		{
			std::swap(data, other.data);
			std::swap(size, other.size);
			std::swap(curr, other.curr);
		}

		char *data = nullptr; /* Start of the buffer. */
		union
		{
			std::size_t pos = 0; /* Next position within the buffer (used for output buffers). */
			std::size_t size;	 /* Size of the buffer (used for input buffers). */
		};
		std::size_t curr = 0; /* Current position within the buffer. */
	};

	template<typename C, typename Traits = std::char_traits<C>>
	class basic_asset_buff : std::basic_streambuf<C, Traits>
	{
		using base_t = std::basic_streambuf<C, Traits>;

	public:
		typedef C char_type;
		typedef Traits traits_type;
		typedef typename traits_type::int_type int_type;
		typedef typename traits_type::pos_type pos_type;
		typedef typename traits_type::off_type off_type;

	private:
		enum status_t : int
		{
			FLAGS_NONE = 0,
			INT_BUFF_OWNED = 1, /* Is the internal buffer owned by us. */
			READING = 2,		/* Buffers are currently used for reading. */
			WRITING = 4,		/* Buffers are currently used for writing. */
		};

		typedef std::ios::openmode openmode;
		typedef typename traits_type::state_type state_type;
		typedef std::codecvt<char, C, state_type> codecvt_t;

		/* By default, external buffer is 8kb, but may be re-allocated as needed. */
		constexpr static std::size_t init_ext_size = SEK_KB(8);

	public:
		constexpr basic_asset_buff(basic_asset_buff &&other) noexcept
			: conv_state_init(std::exchange(other.conv_state_init, {})),
			  conv_state_curr(std::exchange(other.conv_state_curr, {})),
			  conv_state_base(std::exchange(other.conv_state_base, {})),
			  conv(std::exchange(other.conv, nullptr)),
			  int_buff(std::exchange(other.int_buff, int_buff)),
			  int_size(std::exchange(other.int_size, 0)),
			  ext_buff(std::exchange(other.ext_buff, {})),
			  source_file(std::exchange(other.source_file, nullptr)),
			  io_mode(std::exchange(other.io_mode, openmode{})),
			  status(std::exchange(other.status, FLAGS_NONE))
		{
		}

		/** @brief Initializes an asset buffer from a file (always in binary mode).
		 * @param path Path of the asset file.
		 * @param mode Mode used to open the file. Binary mode is always implied. */
		explicit basic_asset_buff(const std::filesystem::path &path, openmode mode = std::ios::in)
		{
			open(path.c_str(), mode);
			cache_codecvt(base_t::getloc());
		}
		/** @brief Initializes an asset from a memory buffer.
		 * @param buff Memory buffer containing asset data.
		 * @param mode Mode used for the stream buffer.
		 * @note While writing to an asset stream buffer backed by a memory buffer is allowed,
		 * changes will not be reflected in the source data used to initialize the memory buffer. */
		explicit basic_asset_buff(asset_membuf &&buff, openmode mode = std::ios::in)
		{
			if (!init_int_buff()) [[unlikely]]
				throw std::bad_alloc();
			cache_codecvt(base_t::getloc());
			ext_buff = std::move(buff);
			io_mode = mode;
		}

		~basic_asset_buff()
		{
			close();
			destroy_int_buff();
		}

		/** Checks if the asset buffer is backed by an open file. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return source_file.is_open(); }

	protected:
		int_type overflow(int_type c = traits_type::eof()) override
		{
			int_type result = traits_type::eof();
			if (io_mode & std::ios::out) [[likely]] {}
			return result;
		}

		void imbue(const std::locale &loc) override
		{
			sync(); /* Sync any pending changes before changing our codecvt. */
			cache_codecvt(loc);
			base_t::imbue(loc);
		}
		int sync() override
		{
			bool success = false;
			if (status & WRITING)
			{
				if (base_t::pptr() != base_t::pbase()) [[likely]]
					success = traits_type::not_eof(overflow(traits_type::eof()));

				if (is_open()) [[likely]]
					success = source_file.sync() && success;
			}
			else if (status & READING)
			{
				/* If buffer is not backed by a file, no sync is needed.
				 * Otherwise, un-read external buffer contents if any and sync the file. */
				if (!(success = !is_open())) [[likely]]
					success = source_file.seek(unwind_read_chars(conv_state_base), 0) >= 0 && source_file.sync();
			}
			return success ? 0 : -1;
		}

	private:
		[[nodiscard]] ssize_t unwind_read_chars(state_type &conv_state)
		{
			/* Calculate negative offset from the current file position used to "unwind" the read buffer. */
			if (conv->always_noconv()) /* If no conversion is needed */
				return static_cast<ssize_t>(ext_buff.pos) - static_cast<ssize_t>(ext_buff.curr);
			else
			{
				/* Get the amount of external characters (from the external buffer)
				 * used to produce the current get area buffer. */
				const auto ext_data = ext_buff.data, ext_end = ext_buff.data + ext_buff.curr;
				const auto ext_count = conv->length(conv_state, ext_data, ext_end, base_t::gptr() - base_t::eback());
				return ext_count - static_cast<ssize_t>(ext_buff.pos);
			}
		}

		void cache_codecvt(const std::locale &loc)
		{
			if (std::has_facet<codecvt_t>(loc)) [[likely]]
				conv = &std::use_facet<codecvt_t>();
			else
				conv = &std::use_facet<codecvt_t>(std::locale{});
			conv_state_init = conv_state_curr = conv_state_base = {};
		}

		void open(const typename std::filesystem::path::value_type *path, openmode mode)
		{
			const auto native_mode =
				(mode & std::ios::in ? detail::native_in : 0) | (mode & std::ios::out ? detail::native_out : 0) |
				(mode & std::ios::app ? detail::native_append : 0) | (mode & std::ios::trunc ? detail::native_trunc : 0) |
#ifdef __cpp_lib_ios_noreplace
				(mode & std::ios::noreplace ? 0 : detail::native_create);
#else
				detail::native_create;
#endif
			if (!is_open() && native_mode != 0 && source_file.open(path, native_mode))
			{
				if ((!int_buff && !init_int_buff()) || ((mode & std::ios::ate) && source_file.seek(0, 1) < 0))
					source_file.close();
				else
					io_mode = mode;
			}
		}
		void close()
		{
			if (status & WRITING) [[likely]]
				base_t::overflow(traits_type::eof());
			if (is_open()) [[likely]]
				source_file.close();
		}

		bool init_int_buff()
		{
			int_buff = static_cast<char_type *>(malloc((int_size = init_ext_size) * sizeof(char_type)));
			if (int_buff != nullptr) [[likely]]
			{
				status |= INT_BUFF_OWNED;
				return true;
			}
			else
				return false;
		}
		void destroy_int_buff()
		{
			if (status & INT_BUFF_OWNED) [[likely]]
				free(int_buff);
		}
		void reset_int_buff()
		{
			destroy_int_buff();
			int_buff = nullptr;
			int_size = 0;
		}

		state_type conv_state_init = {};
		state_type conv_state_curr = {};
		state_type conv_state_base = {};
		const codecvt_t *conv;

		/* Buffer used for internal characters. */
		char_type *int_buff;
		std::size_t int_size;

		/* Buffer used for external asset data (either explicitly specified or read from source file).
		 * If there is a backing file, buffer is periodically flushed to the file. Otherwise, all data is kept here. */
		asset_membuf ext_buff;

		detail::native_file source_file;
		openmode io_mode;
		status_t status;
	};
}	 // namespace sek