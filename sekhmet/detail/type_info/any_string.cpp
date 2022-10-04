//
// Created by switch_blade on 2022-10-04.
//

#include "any_string.hpp"

#include "type_info.hpp"
#include <fmt/format.h>

namespace sek
{
	// clang-format off
	template SEK_API_EXPORT expected<std::basic_string<char>, std::error_code> any_string::as_str(
		std::nothrow_t, const std::locale &, const typename std::basic_string<char>::allocator_type &) const;
	template SEK_API_EXPORT expected<std::basic_string<wchar_t>, std::error_code> any_string::as_str(
		std::nothrow_t, const std::locale &, const typename std::basic_string<wchar_t>::allocator_type &) const;

	template SEK_API_EXPORT std::basic_string<char> any_string::as_str(
		const std::locale &, const typename std::basic_string<char>::allocator_type &) const;
	template SEK_API_EXPORT std::basic_string<wchar_t> any_string::as_str(
		const std::locale &, const typename std::basic_string<wchar_t>::allocator_type &) const;
	// clang-format on

	const detail::string_type_data *any_string::assert_data(const detail::type_data *data)
	{
		if (data->string_data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::INVALID_TYPE), fmt::format("<{}> is not a string", data->name));
		return data->string_data;
	}

	bool any_string::empty() const { return m_data->empty(m_target.data()); }
	std::size_t any_string::size() const { return m_data->size(m_target.data()); }

	void *any_string::data() { return m_data->data(m_target); }
	const void *any_string::data() const { return m_data->cdata(m_target); }
}	 // namespace sek