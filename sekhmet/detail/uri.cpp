/*
 * Created by switchblade on 14/08/22
 */

#include "uri.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#if !defined(SEK_USE_WIN_PATH) && defined(SEK_OS_WIN)
#define SEK_USE_WIN_PATH
#endif

namespace sek
{
	constexpr static typename uri::string_view_type auth_prefix = "//";
	constexpr static typename uri::value_type scheme_postfix = ':';
	constexpr static typename uri::value_type password_prefix = ':';
	constexpr static typename uri::value_type userinfo_postfix = '@';
	constexpr static typename uri::value_type port_prefix = ':';
	constexpr static typename uri::value_type path_separator = '/';
	constexpr static typename uri::value_type query_prefix = '?';
	constexpr static typename uri::value_type fragment_prefix = '#';

	struct uri::data_handle::impl
	{
		[[nodiscard]] constexpr auto &scheme() noexcept { return components[0]; }
		[[nodiscard]] constexpr auto &scheme() const noexcept { return components[0]; }

		[[nodiscard]] constexpr auto &authority() noexcept { return components[1]; }
		[[nodiscard]] constexpr auto &authority() const noexcept { return components[1]; }
		[[nodiscard]] constexpr auto &username() noexcept { return components[2]; }
		[[nodiscard]] constexpr auto &username() const noexcept { return components[2]; }
		[[nodiscard]] constexpr auto &password() noexcept { return components[3]; }
		[[nodiscard]] constexpr auto &password() const noexcept { return components[3]; }
		[[nodiscard]] constexpr auto &host() noexcept { return components[4]; }
		[[nodiscard]] constexpr auto &host() const noexcept { return components[4]; }
		[[nodiscard]] constexpr auto &port() noexcept { return components[5]; }
		[[nodiscard]] constexpr auto &port() const noexcept { return components[5]; }

		[[nodiscard]] constexpr auto &path() noexcept { return components[6]; }
		[[nodiscard]] constexpr auto &path() const noexcept { return components[6]; }
		[[nodiscard]] constexpr auto &filename() noexcept { return components[7]; }
		[[nodiscard]] constexpr auto &filename() const noexcept { return components[7]; }

		[[nodiscard]] constexpr auto &query() noexcept { return components[8]; }
		[[nodiscard]] constexpr auto &query() const noexcept { return components[8]; }

		[[nodiscard]] constexpr auto &fragment() noexcept { return components[9]; }
		[[nodiscard]] constexpr auto &fragment() const noexcept { return components[9]; }

		[[nodiscard]] constexpr bool has_components(uri_component mask) const noexcept
		{
			return (flags & mask) == mask;
		}
		constexpr void offset_components(uri_component mask, size_type off) noexcept
		{
			if (off != 0)
			{
				if ((mask & uri_component::SCHEME) == uri_component::SCHEME) scheme() += off;

				if ((mask & uri_component::AUTHORITY_MASK) == uri_component::AUTHORITY_MASK)
				{
					authority() += off;
					username() += off;
					password() += off;
					host() += off;
					port() += off;
				}

				if ((mask & uri_component::PATH_MASK) == uri_component::PATH_MASK)
				{
					path() += off;
					filename() += off;
				}

				if ((mask & uri_component::QUERY) == uri_component::QUERY) query() += off;
				if ((mask & uri_component::FRAGMENT) == uri_component::FRAGMENT) fragment() += off;
			}
		}

		uri_component flags = {};
		component_t components[10] = {};
	};

	uri::data_handle::data_handle(const data_handle &other) : m_ptr(other.m_ptr ? new impl(*other.m_ptr) : nullptr) {}
	uri::data_handle &uri::data_handle::operator=(const data_handle &other)
	{
		if (this != &other)
		{
			if (m_ptr == nullptr)
				m_ptr = other.m_ptr ? new impl(*other.m_ptr) : nullptr;
			else if (other.m_ptr == nullptr)
				delete std::exchange(m_ptr, nullptr);
			else
				*m_ptr = *other.m_ptr;
		}
		return *this;
	}
	uri::data_handle::~data_handle() { delete m_ptr; }

	uri::data_handle::impl &uri::data_handle::get()
	{
		if (m_ptr == nullptr) [[unlikely]]
			m_ptr = new impl();
		return *m_ptr;
	}

	[[nodiscard]] constexpr std::uint32_t utf8_cp_extract(uri::string_view_type chars) noexcept
	{
		// clang-format off
		switch (chars.length())
		{
			case 1: return static_cast<std::uint32_t>(chars[0]);
			case 2:
				return static_cast<std::uint32_t>(chars[0] & 0b0001'1111) << 6 |
					   static_cast<std::uint32_t>(chars[1] & 0b0011'1111);
			case 3:
				return static_cast<std::uint32_t>(chars[0] & 0b0000'1111) << 12 |
					   static_cast<std::uint32_t>(chars[1] & 0b0011'1111) << 6 |
					   static_cast<std::uint32_t>(chars[2] & 0b0011'1111);
			case 4:
				return static_cast<std::uint32_t>(chars[0] & 0b0000'0111) << 18 |
					   static_cast<std::uint32_t>(chars[1] & 0b0011'1111) << 12 |
					   static_cast<std::uint32_t>(chars[2] & 0b0011'1111) << 6 |
					   static_cast<std::uint32_t>(chars[3] & 0b0011'1111);
			default: return '\0'; /* Invalid. */
		}
		// clang-format on
	}
	[[nodiscard]] constexpr std::uint32_t utf8_cp_width(uri::value_type c) noexcept
	{
		if ((c & 0b1111'1000) == 0b1111'0000)
			return 4;
		else if ((c & 0b1111'0000) == 0b1110'0000)
			return 3;
		else if ((c & 0b1110'0000) == 0b1100'0000)
			return 2;
		return 1;
	}
	inline void utf8_cp_convert(uri::string_type &buffer, std::uint32_t cp)
	{
		if (cp >= 0x10000) /* 4 chars */
		{
			buffer.push_back(static_cast<uri::value_type>((cp >> 18) | 0b1111'0000));
			buffer.push_back(static_cast<uri::value_type>(((cp >> 12) & 0b0011'1111) | 0b1000'0000));
			buffer.push_back(static_cast<uri::value_type>(((cp >> 6) & 0b0011'1111) | 0b1000'0000));
			buffer.push_back(static_cast<uri::value_type>((cp & 0b0011'1111) | 0b1000'0000));
		}
		else if (cp >= 0x800) /* 3 chars */
		{
			buffer.push_back(static_cast<uri::value_type>((cp >> 12) | 0b1110'0000));
			buffer.push_back(static_cast<uri::value_type>(((cp >> 6) & 0b0011'1111) | 0b1000'0000));
			buffer.push_back(static_cast<uri::value_type>((cp & 0b0011'1111) | 0b1000'0000));
		}
		else if (cp >= 80) /* 2 chars */
		{
			buffer.push_back(static_cast<uri::value_type>((cp >> 6) | 0b1100'0000));
			buffer.push_back(static_cast<uri::value_type>((cp & 0b0011'1111) | 0b1000'0000));
		}
		else
			buffer.push_back(static_cast<uri::value_type>(cp));
	}

	[[nodiscard]] constexpr uri::value_type base36_encode(std::size_t digit) noexcept
	{
		constexpr uri::string_view_type alphabet = "abcdefghijklmnopqrstuvwxyz0123456789";
		return alphabet[digit];
	}
	[[nodiscard]] constexpr std::size_t base36_decode(uri::value_type digit) noexcept
	{
		return digit <= '9' ? static_cast<std::size_t>(digit - '0') + 26 :  /* 0-9 */
			   digit <= 'A' ? static_cast<std::size_t>(digit - 'A') :       /* A-Z */
							  static_cast<std::size_t>(digit - 'a');        /* a-z */
	}

	/* Punycode implementation as specified in `RFC 3492` */
	constexpr std::size_t puny_base = 36;
	constexpr std::size_t puny_tmin = 1;
	constexpr std::size_t puny_tmax = 26;
	constexpr std::size_t puny_damp = 700;

	[[nodiscard]] constexpr std::size_t puny_adapt_delta(std::size_t delta, std::size_t n, std::size_t damp) noexcept
	{
		delta = delta / damp;
		delta += delta / n;

		std::size_t k = 0;
		while (delta > ((puny_base - puny_tmin) * puny_tmax) / 2)
		{
			delta /= puny_base - puny_tmin;
			k += puny_base;
		}
		return k + (((puny_base - puny_tmin + 1) * delta) / (delta + 38));
	}
	inline bool puny_encode(uri::string_type &out_buff, std::u32string &cp_buff, uri::string_view_type data)
	{
		out_buff.clear();
		cp_buff.clear();

		/* Copy ASCII characters & extract code points. */
		std::uint32_t n_ascii = 0;
		for (std::uint32_t n, i = 0; i < data.size(); i += n)
		{
			const auto c = data[i];
			n = utf8_cp_width(c);

			/* Copy ASCII directly to the output/ */
			if (n == 1) out_buff.insert(n_ascii++, 1, c);

			/* Extract the next codepoint into the codepoint buffer. */
			cp_buff.push_back(utf8_cp_extract(data.substr(i, n)));
		}

		/* Encode non-ASCII code points. */
		char32_t n = 0x80;
		for (std::size_t i = n_ascii, delta = 0, bias = 72; i < cp_buff.size(); ++delta, ++n)
		{
			/* Find the next minimum code point. */
			auto cp_min = std::numeric_limits<char32_t>::max();
			for (auto code_point : cp_buff)
				if (code_point >= n && code_point < cp_min) cp_min = code_point;

			/* Increase delta enough to advance the decoder's <n,i> state to <m,0> */
			delta += (cp_min - n) * (i + 1);
			n = cp_min;

			/* Calculate & output the delta for the minimum code point. */
			for (auto code_point : cp_buff)
			{
				if (code_point < n)
					++delta;
				else if (code_point == n)
				{
					/* Convert delta to base36. */
					auto q = delta;
					for (std::size_t t, k = puny_base;; k += puny_base)
					{
						t = k <= bias ? puny_tmin : (k >= bias + puny_tmax ? puny_tmax : k - bias);
						if (q < t) break;

						out_buff.push_back(base36_encode(t + (q - t) % (puny_base - t)));
						q = (q - t) / (puny_base - t);
					}
					out_buff.push_back(base36_encode(q));

					/* Finalize the delta. */
					const auto is_first_char = n_ascii == i++;
					bias = puny_adapt_delta(delta, i, (is_first_char ? puny_damp : 2));
					delta = 0;
				}
			}
		}

		/* If the size of the ascii sequence is not 0, insert the ASCII separator. */
		if (n_ascii != 0) out_buff.insert(n_ascii++, 1, '-');
		/* If the size of the ASCII sequence is the same as the size of the string, there are no encoded characters. */
		return n_ascii == out_buff.size();
	}
	inline void puny_decode(uri::string_type &out_buff, std::u32string &cp_buff, uri::string_view_type data)
	{
		out_buff.clear();
		cp_buff.clear();

		/* Handle the basic code points:  Let b be the number of input code */
		/* points before the last delimiter, or 0 if there is none, then    */
		/* copy the first b code points to the output.                      */

		/* Copy all ASCII characters to the codepoint buffer. */
		std::size_t start = 0;
		if (const auto n_ascii = data.find_last_of('-'); n_ascii < data.size())
		{
			for (auto c : data) cp_buff.push_back(static_cast<char32_t>(c));
			start = n_ascii;
		}

		/* `in` is the index of the next input character, `out` is the number of code points written. */
		char32_t n = 0x80;
		for (std::size_t bias = 72, i = 0, in = start, next, out = start; in < data.size(); out = next)
		{
			next = out + 1;

			/* Decode base36 into delta. */
			std::size_t i_offset = 0;
			for (std::size_t t, w = 1, k = puny_base;; k += puny_base, w *= (puny_base - t))
			{
				const auto digit = base36_decode(data[in++]);

				t = k <= bias ? puny_tmin : k >= bias + puny_tmin ? puny_tmin : k - bias;
				i_offset += digit * w;

				if (digit < t) break;
			}

			bias = puny_adapt_delta(i_offset, next, (i ? 2 : puny_damp));
			i += i_offset;
			n += static_cast<char32_t>(i / next);
			i %= next;

			cp_buff.insert(i++, 1, n);
		}

		/* Convert codepoint buffer into result buffer. */
		for (auto cp : cp_buff) utf8_cp_convert(out_buff, cp);
	}

	uri::string_type uri::encode_ace(string_view_type str)
	{
		/* Steps to encode a host string using ACE encoding as defined by Unicode Technical Standard #46:
		 * 1. Separate host into labels at U+002E FULL STOP.
		 * 2. For each label:
		 *      2.1. Skip if the label does not contain non-ASCII Unicode.
		 *      2.2. Encode the label using Punycode
		 *      2.3. Prefix the label with `xn--`.
		 * 3. Join labels using U+002E FULL STOP. */

		/* Allocate enough characters for the work buffers & result string. */
		uri::string_type label_buffer, result;
		std::u32string cp_buffer;
		label_buffer.reserve(str.size());
		cp_buffer.reserve(str.size());
		result.reserve(str.size());

		/* Go through each label & attempt to encode it. Output encoding only if there are any non-ASCII characters. */
		for (std::size_t base = 0, i = 0; base < str.size();)
			if (const auto j = i++; i == str.size() || str[i] == '\x2e')
			{
				/* Encode the label. */
				const auto label = str.substr(base, j - base);
				if (puny_encode(label_buffer, cp_buffer, label)) /* Use punycode-encoded label if there are any unicode characters. */
				{
					result.insert(result.size(), "xn--");
					result.insert(result.size(), label_buffer);
				}
				else
					result.insert(result.size(), label);

				/* If the current label is not the last one, add delimiter.  */
				if (i != str.size()) [[likely]]
					result.push_back('\x2e');

				/* Skip the separator & advance to the next label. */
				base = ++i;
			}
		return result;
	}
	uri::string_type uri::decode_ace(string_view_type str)
	{
		/* Steps to decode a host string using ACE encoding as defined by Unicode Technical Standard #46:
		 * 1. Separate host into labels at U+002E FULL STOP.
		 * 2. For each label:
		 *      2.1. Skip if the label does not start with `xn--`.
		 *      2.2. Decode the label using Punycode
		 * 3. Join labels using U+002E FULL STOP. */

		/* Allocate enough characters for the work buffers & result string. */
		uri::string_type label_buffer, result;
		std::u32string cp_buffer;
		label_buffer.reserve(str.size());
		cp_buffer.reserve(str.size());
		result.reserve(str.size());

		/* Go through each label & decode it if it starts with `xn--`. */
		for (std::size_t base = 0, i = 0; base < str.size();)
			if (const auto j = i++; i == str.size() || str[i] == '\x2e')
			{
				if (const auto label = str.substr(base, j - base); label.starts_with("xn--"))
				{
					puny_decode(label_buffer, cp_buffer, label.substr(4));
					result.insert(result.size(), label_buffer);
				}
				else
					result.insert(result.size(), label);

				/* If the current label is not the last one, add delimiter.  */
				if (i != str.size()) [[likely]]
					result.push_back('\x2e');

				/* Skip the separator & advance to the next label. */
				base = ++i;
			}
		return result;
	}

	inline void decode_ace_host(uri &target)
	{
		/* Decode & replace host of the URI. */
		const auto encoded = target.host();
		const auto decoded = uri::decode_ace(encoded);
		target.set_host(decoded);
	}

	uri uri::from_ace(string_view_type str)
	{
		auto result = uri{str};
		decode_ace_host(result);
		return result;
	}
	uri uri::from_ace(const string_type &str)
	{
		auto result = uri{str};
		decode_ace_host(result);
		return result;
	}
	uri uri::from_ace(string_type &&str)
	{
		auto result = uri{std::forward<string_type>(str)};
		decode_ace_host(result);
		return result;
	}

	[[nodiscard]] constexpr bool is_allowed_separator(uri::value_type c) noexcept
	{
#ifdef SEK_USE_WIN_PATH
		return c == '/' || c == '\\';
#else
		return c == '/';
#endif
	}
	[[nodiscard]] constexpr bool has_root_path(auto begin, auto end) noexcept
	{
		if (const auto size = std::distance(begin, end); size != 0)
		{
#ifdef SEK_USE_WIN_PATH
			return (size > 1 && begin[0] >= 'A' && begin[0] <= 'Z' && (begin[1] == ':' || begin[1] == '|')) ||
				   is_allowed_separator(begin[0]);
#else
			return is_allowed_separator(begin[0]);
#endif
		}
		return false;
	}
	[[nodiscard]] constexpr bool has_filename(auto begin, auto end) noexcept
	{
		for (auto item = end; item-- != begin;)
		{
			/* Filename is the last part of the path not ending with a separator. */
			if (is_allowed_separator(*item)) [[unlikely]]
				return false;
		}
		return begin != end;
	}
	inline void format_local_uri(uri::string_type &uri_str)
	{
		/* Absolute paths must begin with `file://` */
		if (has_root_path(uri_str.begin(), uri_str.end()))
			uri_str.insert(0, "file://");
		else
			uri_str.insert(0, "file:");
	}

	uri uri::from_local(string_view_type path)
	{
		uri result;

		result.m_value = path;
		format_local_uri(result.m_value);
		result.parse();

		return result;
	}
	uri uri::from_local(const string_type &path) { return from_local(string_view_type{path}); }
	uri uri::from_local(string_type &&path)
	{
		format_local_uri(path);
		return uri{std::forward<string_type>(path)};
	}

	void uri::parse() {}

	bool uri::has_components(uri_component mask) const noexcept { return m_data && m_data->has_components(mask); }
	bool uri::is_local() const noexcept { return has_scheme() && m_value.starts_with("file:"); }

	uri &uri::set_filename(string_view_type value)
	{
		size_type offset = 0;
		if (auto &filename = m_data.get().query(); value.empty()) /* Erase the old filename. */
		{
			if (m_data->has_components(uri_component::FILENAME))
			{
				m_value.erase(filename.start, filename.end);
				offset = filename.end - filename.start;

				/* Erase or resize the path component. */
				if (auto &path = m_data->path(); filename == path)
				{
					m_data->flags &= ~uri_component::PATH;
					path = {};
				}
				else
					path.end -= offset;

				m_data->flags &= ~uri_component::FILENAME;
				filename = {};
			}
		}
		else
		{
			/* Create new filename and/or path if needed. */
			if (!m_data->has_components(uri_component::FILENAME))
			{
				/* Filename is inserted either at the end of the string or before query or fragment. */
				size_type pos = m_value.size();
				if (m_data->has_components(uri_component::QUERY))
					pos = m_data->query().start;
				else if (m_data->has_components(uri_component::FRAGMENT))
					pos = m_data->fragment().start;

				/* If there is no path, create it. Otherwise, append a separator. */
				if (!m_data->has_components(uri_component::PATH))
				{
					m_data->path() = {pos, value.size()};
					m_data->flags |= uri_component::PATH;
				}
				else
				{
					m_value.insert(pos++, 1, path_separator);
					offset = 1;
				}

				m_data->filename() = {pos, value.size()};
				m_data->flags |= uri_component::FILENAME;
			}

			/* Insert or erase characters to create buffer space. */
			if (const auto old_size = filename.end - filename.start; old_size < value.size())
			{
				const auto diff = value.size() - old_size;
				m_value.insert(filename.end, diff, '\0');
				m_data->path().end += diff;
				filename.end += diff;
				offset += diff;
			}
			else if (old_size > value.size())
			{
				const auto diff = old_size - value.size();
				m_value.erase(filename.end -= diff, diff);
				m_data->path().end -= diff;
				offset -= diff;
			}
		}

		/* Move query & fragment components. */
		m_data->offset_components(uri_component::QUERY | uri_component::FRAGMENT, offset);
		return *this;
	}
	uri &uri::set_query(string_view_type value)
	{
		size_type offset = 0;
		if (auto &query = m_data.get().query(); value.empty()) /* Erase the old query. */
		{
			if (m_data->has_components(uri_component::QUERY))
			{
				m_value.erase(query.start - 1, query.end); /* Include the prefix. */
				m_data->flags &= ~uri_component::QUERY;
				offset = query.end - query.start;
				query = {};
			}
		}
		else
		{
			/* Create new query if needed. */
			if (!m_data->has_components(uri_component::QUERY))
			{
				/* Insert position is either the start of the fragment or the end of the string. */
				auto pos = m_data->has_components(uri_component::FRAGMENT) ? m_data->fragment().start : m_value.size();
				m_value.insert(pos++, 1, query_prefix);
				m_data->query() = {pos, value.size()};
				m_data->flags |= uri_component::QUERY;
				offset = 1;
			}

			/* Insert or erase characters to create buffer space. */
			if (const auto old_size = query.end - query.start; old_size < value.size())
			{
				const auto diff = value.size() - old_size;
				m_value.insert(query.end, diff, '\0');
				query.end += diff;
				offset += diff;
			}
			else if (old_size > value.size())
			{
				const auto diff = old_size - value.size();
				m_value.erase(query.end -= diff, diff);
				offset -= diff;
			}

			/* Copy string contents into padded buffer space. */
			std::copy_n(value.data(), value.size(), m_value.data() + query.start);
		}

		/* Move fragment component. */
		m_data->offset_components(uri_component::FRAGMENT, offset);
		return *this;
	}
	uri &uri::append_query(string_view_type value, value_type sep)
	{
		/* Set new query if needed. */
		if (auto &query = m_data.get().query(); !m_data->has_components(uri_component::QUERY)) [[unlikely]]
			return set_query(value);
		else
		{
			/* Insert query data & move component end index. */
			const auto old_end = query.end;
			m_value.insert(query.end++, 1, sep);
			m_value.insert(query.end, value);
			query.end += value.size();

			/* Move fragment component if needed. */
			m_data->offset_components(uri_component::FRAGMENT, query.end - old_end);
			return *this;
		}
	}
	uri &uri::set_fragment(string_view_type value)
	{
		if (auto &fragment = m_data.get().fragment(); value.empty()) /* Erase the old fragment. */
		{
			if (m_data->has_components(uri_component::FRAGMENT))
			{
				m_value.erase(fragment.start - 1, fragment.end); /* Include the prefix. */
				m_data->flags &= ~uri_component::FRAGMENT;
				fragment = {};
			}
		}
		else
		{
			/* Create new fragment if needed. */
			if (!m_data->has_components(uri_component::FRAGMENT))
			{
				m_value.push_back(fragment_prefix);
				m_data->fragment() = {m_value.size(), value.size()};
				m_data->flags |= uri_component::FRAGMENT;
			}

			/* Insert or erase characters to create buffer space. */
			if (const auto old_size = fragment.end - fragment.start; old_size < value.size())
			{
				const auto diff = value.size() - old_size;
				m_value.insert(fragment.end, diff, '\0');
				fragment.end += diff;
			}
			else if (old_size > value.size())
			{
				const auto diff = old_size - value.size();
				m_value.erase(fragment.end -= diff, diff);
			}

			/* Copy string contents into padded buffer space. */
			std::copy_n(value.data(), value.size(), m_value.data() + fragment.start);
		}
		return *this;
	}

	/* TODO: Finish this. */
}	 // namespace sek