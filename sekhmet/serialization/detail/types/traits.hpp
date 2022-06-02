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
 * Created by switchblade on 20/05/22
 */

#pragma once

#include "../archive_traits.hpp"
#include "ranges.hpp"
#include "tuples.hpp"

namespace sek::serialization
{
	// clang-format off
	template<typename T, typename A, typename... Args>
	concept adl_serializable = requires(T &&v, A &ar, Args &&...args) { serialize(v, ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept member_serializable = requires(T &&v, A &ar, Args &&...args) { v.serialize(ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept serializable = adl_serializable<T, A, Args...> || member_serializable<T, A, Args...>;

	template<typename T, typename A, typename... Args>
	concept adl_deserializable = requires(T &&v, A &ar, Args &&...args) { deserialize(v, ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept member_deserializable = requires(T &&v, A &ar, Args &&...args) { v.deserialize(ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept deserializable = adl_deserializable<T, A, Args...> || member_deserializable<T, A, Args...>;
	template<typename T, typename A, typename... Args>
	concept in_place_deserializable = requires(A &ar, Args &&...args) { deserialize(std::in_place_type<T>, ar, std::forward<Args>(args)...); };
	// clang-format on
}	 // namespace sek::serialization