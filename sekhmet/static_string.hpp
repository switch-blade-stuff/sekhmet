/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2022-03-27
 */

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