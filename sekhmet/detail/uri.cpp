/*
 * Created by switchblade on 14/08/22
 */

#include "uri.hpp"

#include <utility>

#include <string_view>

#if !defined(SEK_USE_WIN_PATH) && defined(SEK_OS_WIN)
#define SEK_USE_WIN_PATH
#endif

namespace sek
{
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

	[[nodiscard]] constexpr static std::uint32_t utf8_code_point(uri::string_view_type chars) noexcept
	{
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
			default: return 0; /* Invalid. */
		}
	}
	[[nodiscard]] constexpr static std::size_t utf8_width(char c) noexcept
	{
		if ((c & 0b1111'1000) == 0b1111'0000)
			return 4;
		else if ((c & 0b1111'0000) == 0b1110'0000)
			return 3;
		else if ((c & 0b1110'0000) == 0b1100'0000)
			return 2;
		return 1;
	}

	static const uri::string_view_type base36_digits = "abcdefghijklmnopqrstuvwxyz0123456789";

	inline static void base36_encode(uri::string_type &buffer, std::size_t value)
	{
		for (std::size_t digit, offset = 0;; ++offset)
		{
			/* Calculate the digit & advance the value. */
			digit = value % base36_digits.size();
			value = value / base36_digits.size();

			/* Insert the digit to its target position. */
			buffer.insert(buffer.size() - offset, 1, base36_digits[digit]);

			/* Break only after insertion to handle cases where `value` is 0. */
			if (value == 0) [[unlikely]]
				break;
		}
	}
	inline static bool puny_encode(uri::string_type &buffer, uri::string_view_type data)
	{
		buffer.clear();

		std::size_t ascii_end = 0;
		for (std::size_t i = 0; i < data.size(); ++i)
		{
			/* Copy ASCII characters into the buffer & advance the ASCII end position.
			 * For non-ASCII, encode `i * n` in base36 and append to the buffer. */
			if (const auto c = data[i]; c <= '\x7f')
				buffer.insert(ascii_end++, 1, c);
			else
			{
				auto cp_len = std::min(utf8_width(c), data.size() - i);
				const auto cp = data.substr(i, cp_len--);
				const auto n = utf8_code_point(cp);

				base36_encode(buffer, i * static_cast<std::size_t>(n - 127));

				i += cp_len; /* Skip all encoded characters. */
			}
		}

		/* If the ascii end is not 0 (there are ASCII characters), insert the ASCII postfix. */
		if (ascii_end != 0) buffer.insert(ascii_end++, 1, '-');

		/* If the end of the ASCII sequence is the same as the end of the string, there are no encoded characters. */
		return ascii_end == buffer.size();
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
		uri::string_type result, buffer;
		result.reserve(str.size());
		buffer.reserve(str.size());

		for (std::size_t base = 0, i = 0; base < str.size();)
			if (const auto j = i++; i == str.size() || str[i] == '\x2e')
			{
				/* Encode the label. */
				const auto label = str.substr(base, j);
				if (puny_encode(buffer, label)) /* Use punycode-encoded label if there are any unicode characters. */
				{
					result.insert(result.size(), "xn--");
					result.insert(result.size(), buffer);
				}
				else
					result.insert(result.size(), label);

				/* If there is anything after the current label, add delimiter.  */
				if (i != str.size()) [[likely]]
					result.push_back('\x2e');

				/* Skip the separator & advance to the next label. */
				base = ++i;
			}
		return result;
	}
	uri::string_type uri::decode_ace(string_view_type str) {}

	inline static void decode_ace_host(uri &target)
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

	inline static void format_local_uri(typename uri::string_type &uri_str)
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