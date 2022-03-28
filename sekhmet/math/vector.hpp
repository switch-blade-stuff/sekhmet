//
// Created by switchblade on 2021-12-29.
//

#pragma once

#include "detail/vector.hpp"
#include "detail/vectorfwd.hpp"

namespace sek::math
{
	typedef vector<double, 4> vector4d;
	typedef vector<double, 3> vector3d;
	typedef vector<double, 2> vector2d;
	typedef vector<float, 4> vector4f;
	typedef vector<float, 3> vector3f;
	typedef vector<float, 2> vector2f;
	typedef vector<std::uint64_t, 4> vector4ul;
	typedef vector<std::uint64_t, 3> vector3ul;
	typedef vector<std::uint64_t, 2> vector2ul;
	typedef vector<std::int64_t, 4> vector4l;
	typedef vector<std::int64_t, 3> vector3l;
	typedef vector<std::int64_t, 2> vector2l;
	typedef vector<std::uint32_t, 4> vector4ui;
	typedef vector<std::uint32_t, 3> vector3ui;
	typedef vector<std::uint32_t, 2> vector2ui;
	typedef vector<std::int32_t, 4> vector4i;
	typedef vector<std::int32_t, 3> vector3i;
	typedef vector<std::int32_t, 2> vector2i;
}	 // namespace sek::math