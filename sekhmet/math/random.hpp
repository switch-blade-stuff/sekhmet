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
 * Created by switchblade on 2022-03-14
 */

#pragma once

#include "detail/sysrandom.hpp"
#include "detail/xoroshiro.hpp"

namespace sek::math
{
	/** 256-bit version of xoroshiro. */
	template<typename T>
	using xoroshiro256 = xoroshiro<T, 256>;
	/** 128-bit version of xoroshiro. */
	template<typename T>
	using xoroshiro128 = xoroshiro<T, 128>;
}	 // namespace sek::math