//
// Created by switchblade on 2021-12-29.
//

#pragma once

#include "detail/vector.hpp"
#include "detail/vectorfwd.hpp"

namespace sek::math
{
	typedef basic_vector<double, 4> vector4d;
	typedef basic_vector<double, 3> vector3d;
	typedef basic_vector<double, 2> vector2d;
	typedef basic_vector<float, 4> vector4f;
	typedef basic_vector<float, 3> vector3f;
	typedef basic_vector<float, 2> vector2f;
	typedef basic_vector<std::uint64_t, 4> vector4ul;
	typedef basic_vector<std::uint64_t, 3> vector3ul;
	typedef basic_vector<std::uint64_t, 2> vector2ul;
	typedef basic_vector<std::int64_t, 4> vector4l;
	typedef basic_vector<std::int64_t, 3> vector3l;
	typedef basic_vector<std::int64_t, 2> vector2l;
	typedef basic_vector<std::uint32_t, 4> vector4ui;
	typedef basic_vector<std::uint32_t, 3> vector3ui;
	typedef basic_vector<std::uint32_t, 2> vector2ui;
	typedef basic_vector<std::int32_t, 4> vector4i;
	typedef basic_vector<std::int32_t, 3> vector3i;
	typedef basic_vector<std::int32_t, 2> vector2i;
}	 // namespace sek::math