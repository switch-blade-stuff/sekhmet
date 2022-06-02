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
 * Created by switchblade on 2021-12-29
 */

#pragma once

#include "detail/vector.hpp"

namespace sek::math
{
	typedef basic_vector<double, 4> vector4d;
	typedef basic_vector<double, 4, storage_policy::PACKED> vector4d_packed;
	typedef basic_vector<double, 3> vector3d;
	typedef basic_vector<double, 3, storage_policy::PACKED> vector3d_packed;
	typedef basic_vector<double, 2> vector2d;
	typedef basic_vector<double, 2, storage_policy::PACKED> vector2d_packed;
	typedef basic_vector<float, 4> vector4f;
	typedef basic_vector<float, 4, storage_policy::PACKED> vector4f_packed;
	typedef basic_vector<float, 3> vector3f;
	typedef basic_vector<float, 3, storage_policy::PACKED> vector3f_packed;
	typedef basic_vector<float, 2> vector2f;
	typedef basic_vector<float, 2, storage_policy::PACKED> vector2f_packed;
	typedef basic_vector<std::uint64_t, 4> vector4ul;
	typedef basic_vector<std::uint64_t, 4, storage_policy::PACKED> vector4ul_packed;
	typedef basic_vector<std::uint64_t, 3> vector3ul;
	typedef basic_vector<std::uint64_t, 3, storage_policy::PACKED> vector3ul_packed;
	typedef basic_vector<std::uint64_t, 2> vector2ul;
	typedef basic_vector<std::uint64_t, 2, storage_policy::PACKED> vector2ul_packed;
	typedef basic_vector<std::int64_t, 4> vector4l;
	typedef basic_vector<std::int64_t, 4, storage_policy::PACKED> vector4l_packed;
	typedef basic_vector<std::int64_t, 3> vector3l;
	typedef basic_vector<std::int64_t, 3, storage_policy::PACKED> vector3l_packed;
	typedef basic_vector<std::int64_t, 2> vector2l;
	typedef basic_vector<std::int64_t, 2, storage_policy::PACKED> vector2l_packed;
	typedef basic_vector<std::uint32_t, 4> vector4ui;
	typedef basic_vector<std::uint32_t, 4, storage_policy::PACKED> vector4ui_packed;
	typedef basic_vector<std::uint32_t, 3> vector3ui;
	typedef basic_vector<std::uint32_t, 3, storage_policy::PACKED> vector3ui_packed;
	typedef basic_vector<std::uint32_t, 2> vector2ui;
	typedef basic_vector<std::uint32_t, 2, storage_policy::PACKED> vector2ui_packed;
	typedef basic_vector<std::int32_t, 4> vector4i;
	typedef basic_vector<std::int32_t, 4, storage_policy::PACKED> vector4i_packed;
	typedef basic_vector<std::int32_t, 3> vector3i;
	typedef basic_vector<std::int32_t, 3, storage_policy::PACKED> vector3i_packed;
	typedef basic_vector<std::int32_t, 2> vector2i;
	typedef basic_vector<std::int32_t, 2, storage_policy::PACKED> vector2i_packed;
}	 // namespace sek::math