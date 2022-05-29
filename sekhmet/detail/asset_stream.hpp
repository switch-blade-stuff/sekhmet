//
// Created by switchblade on 04/05/22.
//

#pragma once

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <ios>

#include "filemap.hpp"

namespace sek
{
	namespace detail
	{
		struct asset_buffer
		{
			/* Buffer sizes are multiples of 4kb. */
			constexpr static std::size_t size_mult = SEK_KB(4);

			asset_buffer(const asset_buffer &) = delete;
			asset_buffer &operator=(const asset_buffer &) = delete;

			constexpr asset_buffer() noexcept = default;
			constexpr asset_buffer(asset_buffer &&other) noexcept { swap(other); }
			constexpr asset_buffer &operator=(asset_buffer &&other) noexcept
			{
				swap(other);
				return *this;
			}

			explicit asset_buffer(std::size_t n)
			{
				data_begin = data_curr = data_end = static_cast<std::byte *>(malloc(n));
				if (data_begin == nullptr) [[unlikely]]
					throw std::bad_alloc();
			}
			~asset_buffer()
			{
				if (data_begin != nullptr) [[likely]]
					free(data_begin);
			}

			[[nodiscard]] std::size_t write(const void *src, std::size_t n)
			{
				/* Re-allocate the buffer if needed. */
				if (data_curr + n > data_end) [[unlikely]]
				{
					const auto curr_pos = static_cast<std::size_t>(data_curr - data_begin);
					auto new_size = static_cast<std::size_t>(data_end - data_begin) + n;
					const auto rem = new_size % size_mult;
					new_size = new_size - rem + (rem ? size_mult : 0);

					auto new_data = static_cast<std::byte *>(realloc(data_begin, new_size));
					if (new_data == nullptr) [[unlikely]]
						throw std::bad_alloc();

					data_begin = new_data;
					data_curr = new_data + curr_pos;
					data_end = new_data + new_size;
				}
				data_curr = std::copy_n(static_cast<const std::byte *>(src), n, data_curr);
				return n;
			}
			[[nodiscard]] std::size_t read(void *dst, std::size_t n)
			{
				if (data_curr == data_end) [[unlikely]]
					return 0;
				else if (const auto left = static_cast<std::size_t>(data_end - data_curr); left < n) [[unlikely]]
					n = left;
				std::copy_n(data_curr, n, static_cast<std::byte *>(dst));
				data_curr += n;
				return n;
			}
			void reset_pos() noexcept { data_curr = data_begin; }

			constexpr void swap(asset_buffer &other) noexcept
			{
				std::swap(data_begin, other.data_begin);
				std::swap(data_curr, other.data_curr);
				std::swap(data_end, other.data_end);
			}

			std::byte *data_begin = nullptr; /* Start of the buffer. */
			std::byte *data_curr = nullptr;	 /* Current position within the buffer. */
			std::byte *data_end = nullptr;	 /* End of the buffer. */
		};
	}	 // namespace detail

	enum class asset_mode : int
	{
		/** Asset is stored as a loose file. */
		LOOSE,
		/** Asset is stored in an archive with no compression. */
		ARCHIVE_NOCOMP,
		/** Asset is stored in an archive compressed via zstd. */
		ARCHIVE_ZSTD,
	};

	namespace detail
	{
		[[nodiscard]] SEK_API FILE *open_asset_file(const std::filesystem::path &, std::ios::openmode) noexcept;
		[[nodiscard]] SEK_API std::size_t asset_internal_size(std::ios::openmode) noexcept;
	}	 // namespace detail

	template<typename C, asset_mode AssetMode, typename Traits = std::char_traits<C>>
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
		typedef std::ios::openmode openmode;
		typedef typename traits_type::state_type state_type;
		typedef std::codecvt<char, C, state_type> codecvt_t;

		/* By default, external buffer is 8kb and internal is , but may be re-allocated as needed. */
		constexpr static std::size_t init_ext_size = SEK_KB(8);

	public:
		basic_asset_buff(const std::filesystem::path &path, openmode mode)
		{
			init_buff(mode);
			open_impl(path, mode);
			cache_codecvt(base_t::getloc());
		}
		~basic_asset_buff()
		{
			close_impl();
			destroy_buff();
		}

		/** Checks if the asset buffer is open. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return source_file != nullptr; }

	protected:
		void imbue(const std::locale &loc) override
		{
			cache_codecvt(loc);
			base_t::imbue(loc);
		}

	private:
		void cache_codecvt(const std::locale &loc)
		{
			if (std::has_facet<codecvt_t>(loc)) [[likely]]
				conv = &std::use_facet<codecvt_t>();
			else
				conv = &std::use_facet<codecvt_t>(std::locale{});
		}

		void open_impl(const std::filesystem::path &path, openmode mode)
		{
			source_file = detail::open_asset_file(path, mode);
			io_mode = mode;
		}
		void close_impl()
		{
			if (source_file != nullptr) [[likely]]
				fclose(source_file);
		}

		void init_buff(openmode mode)
		{
			if constexpr (AssetMode != asset_mode::LOOSE)
				int_buff_size = detail::asset_internal_size(mode);
			else
				int_buff_size = init_ext_size;
			int_buff = static_cast<char_type *>(malloc(int_buff_size * sizeof(char_type)));
			if (int_buff == nullptr) [[unlikely]]
				throw std::bad_alloc();
		}
		void destroy_buff()
		{
			if (int_buff != nullptr) [[likely]]
				free(int_buff);
			if (ext_buff != nullptr) [[likely]]
				free(ext_buff);
		}

		FILE *source_file;
		openmode io_mode;

		state_type conv_state_init = {};
		state_type conv_state_curr = {};
		state_type conv_state_base = {};
		const codecvt_t *conv;

		/* Buffer used for internal characters. */
		char_type *int_buff;
		std::size_t int_buff_size;

		/* Buffer used for external characters. */
		char *ext_buff = nullptr;
		std::size_t ext_buff_size = 0;
		void *internal_ctx;
	};
}	 // namespace sek