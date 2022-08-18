/*
 * Created by switchblade on 14/08/22
 */

#include "uri.hpp"

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
	constexpr static typename uri::value_type query_prefix = '?';
	constexpr static typename uri::value_type fragment_prefix = '#';

	struct uri::data_handle::impl
	{
		uri_component flags = {};
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

	/* Common variables for punycode. */
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
			n += i / next;
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
				const auto label = str.substr(base, j - base);
				if (label.starts_with("xn--"))
				{
					puny_decode(label_buffer, cp_buffer, label);
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

	[[nodiscard]] constexpr bool is_absolute_path(auto begin, auto end) noexcept
	{
		if (const auto size = std::distance(begin, end); size != 0)
		{
#ifdef SEK_USE_WIN_PATH
			return (size > 1 && begin[0] >= 'A' && begin[0] <= 'Z' && (begin[1] == ':' || begin[1] == '|')) ||
				   (begin[0] == '/' || begin[0] == '\\');
#else
			return begin[0] == '/';
#endif
		}
		return false;
	}
	inline void format_local_uri(typename uri::string_type &uri_str)
	{
		/* Absolute paths must begin with `file://` */
		if (is_absolute_path(uri_str.begin(), uri_str.end()))
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

	bool uri::has_components(uri_component mask) const noexcept { return (m_data->flags & mask) == mask; }
	bool uri::is_local() const noexcept { return m_value.starts_with("file:"); }
}	 // namespace sek