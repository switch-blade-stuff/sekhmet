//
// Created by switchblade on 2022-03-27.
//

#pragma once

#include "detail/static_string.hpp"

namespace sek
{
	template<std::size_t N>
	using static_string = basic_static_string<char, N>;
	template<std::size_t N>
	using static_wstring = basic_static_string<wchar_t, N>;
	template<std::size_t N>
	using static_u8string = basic_static_string<char8_t, N>;
	template<std::size_t N>
	using static_u16string = basic_static_string<char16_t, N>;
	template<std::size_t N>
	using static_u32string = basic_static_string<char32_t, N>;
}