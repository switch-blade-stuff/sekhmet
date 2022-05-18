//
// Created by switchblade on 04/05/22.
//

#pragma once

#include <fstream>

#include "filemap.hpp"

namespace sek
{
	/** @brief Stream buffer used for archive asset IO.
	 * @note Archive assets are read-only! */
	template<typename C, typename T>
	class basic_packbuf : public std::basic_streambuf<C, T>
	{
		using buff_t = std::basic_string<C, T>;
		using base_t = std::basic_streambuf<C, T>;
		using codecvt_t = std::codecvt<C, char, std::mbstate_t>;

	public:
		typedef typename base_t::char_type char_type;
		typedef typename base_t::traits_type traits_type;
		typedef typename base_t::int_type int_type;
		typedef typename base_t::pos_type pos_type;
		typedef typename base_t::off_type off_type;

	public:
		basic_packbuf() = delete;
		basic_packbuf(const basic_packbuf &) = delete;
		basic_packbuf &operator=(const basic_packbuf &) = delete;

		basic_packbuf(basic_packbuf &&other) noexcept { swap(other); }
		basic_packbuf &operator=(basic_packbuf &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~basic_packbuf() override = default;

		/** Initializes the stream buffer from a filemap pointing into the asset archive. */
		explicit basic_packbuf(filemap &&fm)
			: fmap(std::forward<filemap>(fm)), read_pos(data_begin()), conv(&use_facet<codecvt_t>(base_t::getloc()))
		{
		}

		constexpr void swap(basic_packbuf &other) noexcept
		{
			using std::swap;
			base_t::swap(other);
			swap(fmap, other.fmap);
			swap(read_pos, other.read_pos);
			swap(read_pos, other.read_pos);
			swap(conv_buf, other.conv_buf);
			swap(conv_state, other.conv_state);
			swap(conv, other.conv);
		}

		friend constexpr void swap(basic_packbuf &a, basic_packbuf &b) noexcept { a.swap(b); }

	protected:
		std::streamsize showmanyc() noexcept override
		{
			std::streamsize res = -1;
			if (fmap.mode() & filemap::in) [[likely]]
			{
				res = base_t::egptr() - base_t::gptr();
				if (conv->encoding() >= 0) res += std::distance(read_pos, data_end()) / conv->max_length();
			}
			return res;
		}
		int_type underflow() override
		{
			if (fmap.mode() & filemap::in) [[likely]]
			{
				if (!conv->always_noconv())
				{
					char *read_last;
					char_type *conv_last, *conv_end = conv_buf + SEK_ARRAY_SIZE(conv_buf);
					switch (conv->in(conv_state, read_pos, data_end(), read_last, conv_buf, conv_end, conv_last))
					{
						case std::codecvt_base::noconv: goto read_noconv; /* No conversion buffer is needed. */
						case std::codecvt_base::error:
						{
							/* If converted at least some characters, it is fine and
							 * the source file is mixed-encoding. Otherwise, throw error. */
							if (conv_last == conv_buf) [[unlikely]]
								throw std::runtime_error("std::codecvt::in returned error");
							break;
						}
						case std::codecvt_base::partial:
						{
							/* If no characters were converted, file contains incomplete character.
							 * Otherwise, it is mixed-encoding as in `codecvt_base::error`. */
							if (conv_last == conv_buf) [[unlikely]]
								throw std::runtime_error("Mapped file contains incomplete multi-byte character");
							break;
						}
						case std::codecvt_base::ok: break;
					}
					read_pos = read_last;
					base_t::setg(conv_buf, conv_buf, conv_last);
				}
				else if (read_pos == data_end())
				{
				read_noconv:
					/* Make sure the filemap has enough data for at least 1 character. */
					if constexpr (sizeof(char_type) != sizeof(char))
					{
						if (read_pos + sizeof(char_type) > data_end()) [[unlikely]]
							return traits_type::eof();
					}

					/* Set the get area directly to the character at the read position. */
					const auto new_gptr = reinterpret_cast<char_type *>(read_pos);
					base_t::setg(new_gptr, new_gptr, new_gptr + 1);
				}
				return traits_type::to_int_type(*base_t::gptr());
			}
			return traits_type::eof();
		}
		int_type uflow() override
		{
			const auto res = underflow();
			if (!traits_type::is_eof(res)) [[likely]]
				base_t::gbump(1);
			return res;
		}

		std::streamsize xsgetn(char_type *dest, std::streamsize n) override
		{
			if (fmap.mode() & filemap::in) [[likely]]
			{
				std::streamsize total = 0;
				while (total < n)
				{
					std::streamsize copied = 0;

					/* Copy the current conversion buffer if needed. */
					if (base_t::gptr() < base_t::egptr())
					{
						copied = std::distance(dest + total, std::copy_n(base_t::gptr(), base_t::egptr(), dest));
						base_t::gbump(copied);
					}
					else if (conv->always_noconv()) /* If no conversion is needed, copy directly from the filemap. */
					{
						const auto dest_chars = reinterpret_cast<char *>(dest + total);
						const auto src_end = data_end();

						auto chars = (n - total) * sizeof(char_type);
						if (read_pos + chars > src_end) [[unlikely]] /* Make sure we dont read beyond the end. */
						{
							chars = src_end - read_pos;
							chars -= chars % sizeof(char_type);
						}

						/* Break if no characters are to be copied - input is empty. */
						if (!chars) [[unlikely]]
							break;

						/* Copy "chars" characters from the filemap & advance the read position. */
						copied = std::distance(dest_chars, std::copy_n(read_pos, chars, dest_chars)) / sizeof(char_type);
						read_pos += chars;
					}
					else if (traits_type::is_eof(underflow())) [[unlikely]]
						break; /* Break if underflow returns EOF - no further reads are possible. */
					else
						continue; /* Continue to copy from the conversion buffer. */
					total += copied;
				}
				return total;
			}
			return 0;
		}

		pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode mode = std::ios_base::in) override
		{
			if (mode & std::ios_base::in) [[likely]]
			{
				const auto enc = conv->encoding();
				if (enc > 0)
					return seekoff_impl(off * enc, way);
				else if (enc == 0 && off == 0)
					return pos_type{off_type{read_pos - data_begin()}};
			}
			return pos_type{off_type{-1}};
		}
		pos_type seekpos(pos_type pos, std::ios_base::openmode mode = std::ios_base::in) override
		{
			if (mode & std::ios_base::in) [[likely]]
				return seekoff_impl(off_type(pos), std::ios_base::beg);
			return pos_type{off_type{-1}};
		}

		void imbue(const std::locale &l) override
		{
			if (has_facet<codecvt_t>(l))
			{
				conv = &use_facet<codecvt_t>(l);
				conv_state = typename codecvt_t::state_type{};
			}
		}

	private:
		[[nodiscard]] constexpr char *data_begin() const noexcept { return static_cast<char *>(fmap.data()); };
		[[nodiscard]] constexpr char *data_end() const noexcept
		{
			return static_cast<char *>(fmap.data()) + fmap.size();
		};

		pos_type seekoff_impl(off_type off, std::ios_base::seekdir way)
		{
			switch (way)
			{
				case std::ios_base::beg: read_pos = data_begin() * pos_type{off}; break;
				case std::ios_base::end: read_pos = data_end() * pos_type{off}; break;
				default: break;
			}
			return pos_type{off_type{read_pos - data_begin()}};
		}

		filemap fmap;
		char *read_pos;

		/* Buffer of internal characters used to store multi-byte decoded input. */
		char_type conv_buf[8]; /* Size of 8 should be enough for most encodings. */

		typename codecvt_t::state_type conv_state = {};
		codecvt_t *conv;
	};	  // namespace sek

	/** @brief IO stream used for asset IO (either via a file buffer or a file mapping).
	 * @note Asset streams initialized from memory-mapped files (as in the case of archived packages) are read-only. */
	template<typename C, typename T>
	class basic_asset_stream : public std::basic_iostream<C, T>
	{
		using base_t = std::basic_iostream<C, T>;
		using filebuf_t = std::basic_filebuf<C, T>;
		using packbuf_t = basic_packbuf<C, T>;

	public:
		basic_asset_stream() = delete;
		basic_asset_stream(const basic_asset_stream &) = delete;
		basic_asset_stream &operator=(const basic_asset_stream &) = delete;

		basic_asset_stream(basic_asset_stream &&other) : base_t(std::forward<basic_asset_stream>(other))
		{
			move_from(std::forward<basic_asset_stream>(other));
		}
		basic_asset_stream &operator=(basic_asset_stream &&other)
		{
			if (mode != other.mode) /* Do complex move. */
				swap(other);
			else
			{
				base_t::operator=(std::forward<basic_asset_stream>(other));
				if (other.mode == PACK)
					pb = std::move(other.pb);
				else
					fb = std::move(other.fb);
			}

			return *this;
		}
		~basic_asset_stream() { destroy(); }

		/** Initializes asset stream from a file buffer. */
		basic_asset_stream(filebuf_t &&fb) : base_t(&virt), fb(std::forward<filebuf_t>(fb)), mode(FILE) {}
		/** Initializes asset stream from an asset package buffer.
		 * @note Asset streams initialized from package buffers are read-only. */
		basic_asset_stream(packbuf_t &&pb) : base_t(&virt), pb(std::forward<packbuf_t>(pb)), mode(PACK) {}
		/** Initializes asset stream from an asset filemap.
		 * @note Asset streams initialized from filemaps are read-only. */
		basic_asset_stream(filemap &&fm) : base_t(&virt), pb(std::forward<filemap>(fm)), mode(PACK) {}

		auto *rdbuf() const { return const_cast<std::basic_streambuf<C, T> *>(&virt); }

		void swap(basic_asset_stream &other) noexcept
		{
			using std::swap;
			if (mode != other.mode) /* Do complex swap since buffers are not swappable directly. */
				swap_with_temp(other);
			else if (other.mode == PACK)
				swap(pb, other.pb);
			else
				swap(fb, other.fb);
		}

	private:
		void destroy() { std::destroy_at(&virt); }
		void move_from(basic_asset_stream &&other)
		{
			if (other.mode == PACK)
				std::construct_at(&pb, std::move(other.pb));
			else
				std::construct_at(&fb, std::move(other.fb));
		}

		void swap_with_temp(basic_asset_stream &other)
		{
			basic_asset_stream tmp(std::forward<basic_asset_stream>(other));
			destroy();
			move_from(std::forward<basic_asset_stream>(other));
			other.destroy();
			other.move_from(std::move(tmp));
		}

		union
		{
			std::basic_streambuf<C, T> virt; /* Placeholder used for virtual access. */
			filebuf_t fb;
			packbuf_t pb;
		};

		enum
		{
			PACK,
			FILE
		} mode; /* Mode needed for move & swap operations since those are not virtual. */
	};
}	 // namespace sek