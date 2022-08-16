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
#ifdef SEK_USE_WIN_PATH
	constexpr static typename uri::string_view_type path_separator = "\\/";

	/* Allow `|` to be used for paths in non-local URIs. */
	constexpr static typename uri::string_view_type uri_drive_postfix = ":|";
	constexpr static typename uri::value_type drive_postfix = ':';
#else
	constexpr static typename uri::string_view_type path_separator = "/";
#endif
	constexpr static typename uri::value_type normal_separator = '/';

	constexpr static typename uri::string_view_type auth_prefix = "//";
	constexpr static typename uri::value_type scheme_postfix = ':';
	constexpr static typename uri::value_type password_prefix = ':';
	constexpr static typename uri::value_type userinfo_postfix = '@';
	constexpr static typename uri::value_type port_prefix = ':';
	constexpr static typename uri::value_type query_prefix = '?';
	constexpr static typename uri::value_type fragment_prefix = '#';

	struct uri::data_handle::impl
	{
		component_type flags = {};
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

	void uri::parse_components() {}

	bool uri::has_components(component_type mask) const noexcept { return (m_data->flags & mask) == mask; }
	bool uri::is_local() const noexcept { return m_value.starts_with("file:"); }
}	 // namespace sek