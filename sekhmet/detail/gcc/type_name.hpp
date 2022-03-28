//
// Created by switchblade on 2022-01-15.
//

#pragma once

#if defined(__clang__) || defined(__GNUC__)

namespace sek::detail
{
	template<basic_static_string Name>
	consteval auto generate_type_name() noexcept
	{
		constexpr auto offset_start = Name.find_first('=') + 2;
		constexpr auto offset_end = Name.find_last(']');
		constexpr auto trimmed_length = offset_end - offset_start + 1;

		return format_type_name<Name, 0, offset_start, offset_end, trimmed_length>();
	}
}	 // namespace sek

#endif