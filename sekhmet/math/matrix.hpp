/*
 * Created by switchblade on 2021-12-29
 */

#pragma once

#include "detail/matrix.hpp"
#include "detail/matrix2x.hpp"
#include "detail/matrix3x.hpp"
#include "detail/matrix4x.hpp"

namespace sek::math
{
	template<typename T = float>
	using mat4x4 = basic_mat<T, 4, 4>;
	template<typename T = float>
	using mat4x3 = basic_mat<T, 4, 3>;
	template<typename T = float>
	using mat4x2 = basic_mat<T, 4, 2>;
	template<typename T = float>
	using mat3x4 = basic_mat<T, 3, 4>;
	template<typename T = float>
	using mat3x3 = basic_mat<T, 3, 3>;
	template<typename T = float>
	using mat3x2 = basic_mat<T, 3, 2>;
	template<typename T = float>
	using mat2x4 = basic_mat<T, 2, 4>;
	template<typename T = float>
	using mat2x3 = basic_mat<T, 2, 3>;
	template<typename T = float>
	using mat2x2 = basic_mat<T, 2, 2>;
	template<typename T = float>
	using mat4 = mat4x4<T>;
	template<typename T = float>
	using mat3 = mat3x3<T>;
	template<typename T = float>
	using mat2 = mat2x2<T>;

	template<typename T = float>
	using mat4x4_packed = basic_mat<T, 4, 4, storage_policy::PACKED>;
	template<typename T = float>
	using mat4x3_packed = basic_mat<T, 4, 3, storage_policy::PACKED>;
	template<typename T = float>
	using mat4x2_packed = basic_mat<T, 4, 2, storage_policy::PACKED>;
	template<typename T = float>
	using mat3x4_packed = basic_mat<T, 3, 4, storage_policy::PACKED>;
	template<typename T = float>
	using mat3x3_packed = basic_mat<T, 3, 3, storage_policy::PACKED>;
	template<typename T = float>
	using mat3x2_packed = basic_mat<T, 3, 2, storage_policy::PACKED>;
	template<typename T = float>
	using mat2x4_packed = basic_mat<T, 2, 4, storage_policy::PACKED>;
	template<typename T = float>
	using mat2x3_packed = basic_mat<T, 2, 3, storage_policy::PACKED>;
	template<typename T = float>
	using mat2x2_packed = basic_mat<T, 2, 2, storage_policy::PACKED>;
	template<typename T = float>
	using mat4_packed = mat4x4_packed<T>;
	template<typename T = float>
	using mat3_packed = mat3x3_packed<T>;
	template<typename T = float>
	using mat2_packed = mat2x2_packed<T>;

	typedef mat4x4<double> dmat4x4;
	typedef mat4x3<double> dmat4x3;
	typedef mat4x2<double> dmat4x2;
	typedef mat3x4<double> dmat3x4;
	typedef mat3x3<double> dmat3x3;
	typedef mat3x2<double> dmat3x2;
	typedef mat2x4<double> dmat2x4;
	typedef mat2x3<double> dmat2x3;
	typedef mat2x2<double> dmat2x2;
	typedef mat4<double> dmat4;
	typedef mat3<double> dmat3;
	typedef mat2<double> dmat2;
	typedef mat4x4_packed<double> dmat4x4_packed;
	typedef mat4x3_packed<double> dmat4x3_packed;
	typedef mat4x2_packed<double> dmat4x2_packed;
	typedef mat3x4_packed<double> dmat3x4_packed;
	typedef mat3x3_packed<double> dmat3x3_packed;
	typedef mat3x2_packed<double> dmat3x2_packed;
	typedef mat2x4_packed<double> dmat2x4_packed;
	typedef mat2x3_packed<double> dmat2x3_packed;
	typedef mat2x2_packed<double> dmat2x2_packed;
	typedef mat4_packed<double> dmat4_packed;
	typedef mat3_packed<double> dmat3_packed;
	typedef mat2_packed<double> dmat2_packed;

	typedef mat4x4<float> fmat4x4;
	typedef mat4x3<float> fmat4x3;
	typedef mat4x2<float> fmat4x2;
	typedef mat3x4<float> fmat3x4;
	typedef mat3x3<float> fmat3x3;
	typedef mat3x2<float> fmat3x2;
	typedef mat2x4<float> fmat2x4;
	typedef mat2x3<float> fmat2x3;
	typedef mat2x2<float> fmat2x2;
	typedef mat4<float> fmat4;
	typedef mat3<float> fmat3;
	typedef mat2<float> fmat2;
	typedef mat4x4_packed<float> fmat4x4_packed;
	typedef mat4x3_packed<float> fmat4x3_packed;
	typedef mat4x2_packed<float> fmat4x2_packed;
	typedef mat3x4_packed<float> fmat3x4_packed;
	typedef mat3x3_packed<float> fmat3x3_packed;
	typedef mat3x2_packed<float> fmat3x2_packed;
	typedef mat2x4_packed<float> fmat2x4_packed;
	typedef mat2x3_packed<float> fmat2x3_packed;
	typedef mat2x2_packed<float> fmat2x2_packed;
	typedef mat4_packed<float> fmat4_packed;
	typedef mat3_packed<float> fmat3_packed;
	typedef mat2_packed<float> fmat2_packed;

	typedef mat4x4<std::uint64_t> ulmat4x4;
	typedef mat4x3<std::uint64_t> ulmat4x3;
	typedef mat4x2<std::uint64_t> ulmat4x2;
	typedef mat3x4<std::uint64_t> ulmat3x4;
	typedef mat3x3<std::uint64_t> ulmat3x3;
	typedef mat3x2<std::uint64_t> ulmat3x2;
	typedef mat2x4<std::uint64_t> ulmat2x4;
	typedef mat2x3<std::uint64_t> ulmat2x3;
	typedef mat2x2<std::uint64_t> ulmat2x2;
	typedef mat4x4<std::int64_t> lmat4x4;
	typedef mat4x3<std::int64_t> lmat4x3;
	typedef mat4x2<std::int64_t> lmat4x2;
	typedef mat3x4<std::int64_t> lmat3x4;
	typedef mat3x3<std::int64_t> lmat3x3;
	typedef mat3x2<std::int64_t> lmat3x2;
	typedef mat2x4<std::int64_t> lmat2x4;
	typedef mat2x3<std::int64_t> lmat2x3;
	typedef mat2x2<std::int64_t> lmat2x2;
	typedef mat4<std::uint64_t> ulmat4;
	typedef mat3<std::uint64_t> ulmat3;
	typedef mat2<std::uint64_t> ulmat2;
	typedef mat4<std::int64_t> lmat4;
	typedef mat3<std::int64_t> lmat3;
	typedef mat2<std::int64_t> lmat2;
	typedef mat4x4_packed<std::uint64_t> ulmat4x4_packed;
	typedef mat4x3_packed<std::uint64_t> ulmat4x3_packed;
	typedef mat4x2_packed<std::uint64_t> ulmat4x2_packed;
	typedef mat3x4_packed<std::uint64_t> ulmat3x4_packed;
	typedef mat3x3_packed<std::uint64_t> ulmat3x3_packed;
	typedef mat3x2_packed<std::uint64_t> ulmat3x2_packed;
	typedef mat2x4_packed<std::uint64_t> ulmat2x4_packed;
	typedef mat2x3_packed<std::uint64_t> ulmat2x3_packed;
	typedef mat2x2_packed<std::uint64_t> ulmat2x2_packed;
	typedef mat4x4_packed<std::int64_t> lmat4x4_packed;
	typedef mat4x3_packed<std::int64_t> lmat4x3_packed;
	typedef mat4x2_packed<std::int64_t> lmat4x2_packed;
	typedef mat3x4_packed<std::int64_t> lmat3x4_packed;
	typedef mat3x3_packed<std::int64_t> lmat3x3_packed;
	typedef mat3x2_packed<std::int64_t> lmat3x2_packed;
	typedef mat2x4_packed<std::int64_t> lmat2x4_packed;
	typedef mat2x3_packed<std::int64_t> lmat2x3_packed;
	typedef mat2x2_packed<std::int64_t> lmat2x2_packed;
	typedef mat4_packed<std::uint64_t> ulmat4_packed;
	typedef mat3_packed<std::uint64_t> ulmat3_packed;
	typedef mat2_packed<std::uint64_t> ulmat2_packed;
	typedef mat4_packed<std::int64_t> lmat4_packed;
	typedef mat3_packed<std::int64_t> lmat3_packed;
	typedef mat2_packed<std::int64_t> lmat2_packed;

	typedef mat4x4<std::uint32_t> uimat4x4;
	typedef mat4x3<std::uint32_t> uimat4x3;
	typedef mat4x2<std::uint32_t> uimat4x2;
	typedef mat3x4<std::uint32_t> uimat3x4;
	typedef mat3x3<std::uint32_t> uimat3x3;
	typedef mat3x2<std::uint32_t> uimat3x2;
	typedef mat2x4<std::uint32_t> uimat2x4;
	typedef mat2x3<std::uint32_t> uimat2x3;
	typedef mat2x2<std::uint32_t> uimat2x2;
	typedef mat4x4<std::int32_t> imat4x4;
	typedef mat4x3<std::int32_t> imat4x3;
	typedef mat4x2<std::int32_t> imat4x2;
	typedef mat3x4<std::int32_t> imat3x4;
	typedef mat3x3<std::int32_t> imat3x3;
	typedef mat3x2<std::int32_t> imat3x2;
	typedef mat2x4<std::int32_t> imat2x4;
	typedef mat2x3<std::int32_t> imat2x3;
	typedef mat2x2<std::int32_t> imat2x2;
	typedef mat4<std::uint32_t> uimat4;
	typedef mat3<std::uint32_t> uimat3;
	typedef mat2<std::uint32_t> uimat2;
	typedef mat4<std::int32_t> imat4;
	typedef mat3<std::int32_t> imat3;
	typedef mat2<std::int32_t> imat2;
	typedef mat4x4_packed<std::uint32_t> uimat4x4_packed;
	typedef mat4x3_packed<std::uint32_t> uimat4x3_packed;
	typedef mat4x2_packed<std::uint32_t> uimat4x2_packed;
	typedef mat3x4_packed<std::uint32_t> uimat3x4_packed;
	typedef mat3x3_packed<std::uint32_t> uimat3x3_packed;
	typedef mat3x2_packed<std::uint32_t> uimat3x2_packed;
	typedef mat2x4_packed<std::uint32_t> uimat2x4_packed;
	typedef mat2x3_packed<std::uint32_t> uimat2x3_packed;
	typedef mat2x2_packed<std::uint32_t> uimat2x2_packed;
	typedef mat4x4_packed<std::int32_t> imat4x4_packed;
	typedef mat4x3_packed<std::int32_t> imat4x3_packed;
	typedef mat4x2_packed<std::int32_t> imat4x2_packed;
	typedef mat3x4_packed<std::int32_t> imat3x4_packed;
	typedef mat3x3_packed<std::int32_t> imat3x3_packed;
	typedef mat3x2_packed<std::int32_t> imat3x2_packed;
	typedef mat2x4_packed<std::int32_t> imat2x4_packed;
	typedef mat2x3_packed<std::int32_t> imat2x3_packed;
	typedef mat2x2_packed<std::int32_t> imat2x2_packed;
	typedef mat4_packed<std::uint32_t> uimat4_packed;
	typedef mat3_packed<std::uint32_t> uimat3_packed;
	typedef mat2_packed<std::uint32_t> uimat2_packed;
	typedef mat4_packed<std::int32_t> imat4_packed;
	typedef mat3_packed<std::int32_t> imat3_packed;
	typedef mat2_packed<std::int32_t> imat2_packed;
}	 // namespace sek::math