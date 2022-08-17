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

	inline static void encode_ace_str(uri::string_type &str)
	{
		/* Steps to encode a host string using ACE encoding as defined by Unicode Technical Standard #46:
		 * 1. Separate host into labels at U+002E FULL STOP.
		 * 2. For each label:
		 *      2.1. Skip if the label does not contain non-ASCII Unicode.
		 *      2.2. Encode the label using Punycode
		 *      2.3. Prefix the label with `xn--`.
		 * 3. Join labels using U+002E FULL STOP. */
		for (std::size_t base = 0, i = 0, n = str.size(); i < n; ++i)
		{
			/* Found the end of the label. */
			if (i + 1 >= n || str[i] == '\x2e')
			{
				/* Erase all non-ASCII characters. */
				for (std::size_t j = base; j != i; ++j)
				{

				}
			}
		}
	}

	uri::string_type uri::encode_ace(string_view_type str)
	{
		auto result = string_type{str};
		encode_ace_str(result);
		return result;
	}
	uri::string_type uri::encode_ace(const string_type &str)
	{
		auto result = string_type{str};
		encode_ace_str(result);
		return result;
	}
	uri::string_type uri::encode_ace(string_type &&str)
	{
		auto result = string_type{std::forward<string_type>(str)};
		encode_ace_str(result);
		return result;
	}

	inline static void decode_ace_str(uri::string_type &str) {}

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
		result.parse_components();

		return result;
	}
	uri uri::from_local(const string_type &path) { return from_local(string_view_type{path}); }
	uri uri::from_local(string_type &&path)
	{
		format_local_uri(path);
		return uri{std::forward<string_type>(path)};
	}

	void uri::parse_components() {}

	bool uri::has_components(uri_component mask) const noexcept { return (m_data->flags & mask) == mask; }
	bool uri::is_local() const noexcept { return m_value.starts_with("file:"); }
}	 // namespace sek