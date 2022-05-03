//
// Created by switchblade on 2021-12-29.
//

#pragma once

#include "detail/matrix.hpp"
#include "detail/matrix2x.hpp"
#include "detail/matrix3x.hpp"
#include "detail/matrix4x.hpp"

namespace sek::math
{
	typedef basic_matrix<double, 4, 4> matrix4x4d;
	typedef basic_matrix<double, 4, 3> matrix4x3d;
	typedef basic_matrix<double, 4, 2> matrix4x2d;
	typedef basic_matrix<double, 3, 4> matrix3x4d;
	typedef basic_matrix<double, 3, 3> matrix3x3d;
	typedef basic_matrix<double, 3, 2> matrix3x2d;
	typedef basic_matrix<double, 2, 4> matrix2x4d;
	typedef basic_matrix<double, 2, 3> matrix2x3d;
	typedef basic_matrix<double, 2, 2> matrix2x2d;
	typedef matrix4x4d matrix4d;
	typedef matrix3x3d matrix3d;
	typedef matrix2x2d matrix2d;

	typedef basic_matrix<float, 4, 4> matrix4x4f;
	typedef basic_matrix<float, 4, 3> matrix4x3f;
	typedef basic_matrix<float, 4, 2> matrix4x2f;
	typedef basic_matrix<float, 3, 4> matrix3x4f;
	typedef basic_matrix<float, 3, 3> matrix3x3f;
	typedef basic_matrix<float, 3, 2> matrix3x2f;
	typedef basic_matrix<float, 2, 4> matrix2x4f;
	typedef basic_matrix<float, 2, 3> matrix2x3f;
	typedef basic_matrix<float, 2, 2> matrix2x2f;
	typedef matrix4x4f matrix4f;
	typedef matrix3x3f matrix3f;
	typedef matrix2x2f matrix2f;

	typedef basic_matrix<std::uint64_t, 4, 4> matrix4x4ul;
	typedef basic_matrix<std::uint64_t, 4, 3> matrix4x3ul;
	typedef basic_matrix<std::uint64_t, 4, 2> matrix4x2ul;
	typedef basic_matrix<std::uint64_t, 3, 4> matrix3x4ul;
	typedef basic_matrix<std::uint64_t, 3, 3> matrix3x3ul;
	typedef basic_matrix<std::uint64_t, 3, 2> matrix3x2ul;
	typedef basic_matrix<std::uint64_t, 2, 4> matrix2x4ul;
	typedef basic_matrix<std::uint64_t, 2, 3> matrix2x3ul;
	typedef basic_matrix<std::uint64_t, 2, 2> matrix2x2ul;
	typedef basic_matrix<std::int64_t, 4, 4> matrix4x4l;
	typedef basic_matrix<std::int64_t, 4, 3> matrix4x3l;
	typedef basic_matrix<std::int64_t, 4, 2> matrix4x2l;
	typedef basic_matrix<std::int64_t, 3, 4> matrix3x4l;
	typedef basic_matrix<std::int64_t, 3, 3> matrix3x3l;
	typedef basic_matrix<std::int64_t, 3, 2> matrix3x2l;
	typedef basic_matrix<std::int64_t, 2, 4> matrix2x4l;
	typedef basic_matrix<std::int64_t, 2, 3> matrix2x3l;
	typedef basic_matrix<std::int64_t, 2, 2> matrix2x2l;
	typedef matrix4x4ul matrix4ul;
	typedef matrix3x3ul matrix3ul;
	typedef matrix2x2ul matrix2ul;
	typedef matrix4x4l matrix4l;
	typedef matrix3x3l matrix3l;
	typedef matrix2x2l matrix2l;

	typedef basic_matrix<std::uint32_t, 4, 4> matrix4x4ui;
	typedef basic_matrix<std::uint32_t, 4, 3> matrix4x3ui;
	typedef basic_matrix<std::uint32_t, 4, 2> matrix4x2ui;
	typedef basic_matrix<std::uint32_t, 3, 4> matrix3x4ui;
	typedef basic_matrix<std::uint32_t, 3, 3> matrix3x3ui;
	typedef basic_matrix<std::uint32_t, 3, 2> matrix3x2ui;
	typedef basic_matrix<std::uint32_t, 2, 4> matrix2x4ui;
	typedef basic_matrix<std::uint32_t, 2, 3> matrix2x3ui;
	typedef basic_matrix<std::uint32_t, 2, 2> matrix2x2ui;
	typedef basic_matrix<std::int32_t, 4, 4> matrix4x4i;
	typedef basic_matrix<std::int32_t, 4, 3> matrix4x3i;
	typedef basic_matrix<std::int32_t, 4, 2> matrix4x2i;
	typedef basic_matrix<std::int32_t, 3, 4> matrix3x4i;
	typedef basic_matrix<std::int32_t, 3, 3> matrix3x3i;
	typedef basic_matrix<std::int32_t, 3, 2> matrix3x2i;
	typedef basic_matrix<std::int32_t, 2, 4> matrix2x4i;
	typedef basic_matrix<std::int32_t, 2, 3> matrix2x3i;
	typedef basic_matrix<std::int32_t, 2, 2> matrix2x2i;
	typedef matrix4x4ui matrix4ui;
	typedef matrix3x3ui matrix3ui;
	typedef matrix2x2ui matrix2ui;
	typedef matrix4x4i matrix4i;
	typedef matrix3x3i matrix3i;
	typedef matrix2x2i matrix2i;
}	 // namespace sek::math