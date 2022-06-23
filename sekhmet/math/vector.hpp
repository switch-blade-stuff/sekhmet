/*
 * Created by switchblade on 2021-12-29
 */

#pragma once

#include "detail/vector.hpp"

namespace sek::math
{
	template<typename T = float>
	using vec4 = basic_vec<T, 4>;
	template<typename T = float>
	using vec3 = basic_vec<T, 3>;
	template<typename T = float>
	using vec2 = basic_vec<T, 2>;
	template<typename T = float>
	using vec4_packed = basic_vec<T, 4, storage_policy::PACKED>;
	template<typename T = float>
	using vec3_packed = basic_vec<T, 3, storage_policy::PACKED>;
	template<typename T = float>
	using vec2_packed = basic_vec<T, 2, storage_policy::PACKED>;

	typedef vec4<double> dvec4;
	typedef vec3<double> dvec3;
	typedef vec2<double> dvec2;
	typedef vec4<float> fvec4;
	typedef vec3<float> fvec3;
	typedef vec2<float> fvec2;
	typedef vec4_packed<double> dvec4_packed;
	typedef vec3_packed<double> dvec3_packed;
	typedef vec2_packed<double> dvec2_packed;
	typedef vec4_packed<float> fvec4_packed;
	typedef vec3_packed<float> fvec3_packed;
	typedef vec2_packed<float> fvec2_packed;
	typedef vec4<std::uint64_t> ulvec4;
	typedef vec3<std::uint64_t> ulvec3;
	typedef vec2<std::uint64_t> ulvec2;
	typedef vec4<std::int64_t> lvec4;
	typedef vec3<std::int64_t> lvec3;
	typedef vec2<std::int64_t> lvec2;
	typedef vec4<std::uint32_t> uivec4;
	typedef vec3<std::uint32_t> uivec3;
	typedef vec2<std::uint32_t> uivec2;
	typedef vec4<std::int32_t> ivec4;
	typedef vec3<std::int32_t> ivec3;
	typedef vec2<std::int32_t> ivec2;
	typedef vec4_packed<std::uint64_t> ulvec4_packed;
	typedef vec3_packed<std::uint64_t> ulvec3_packed;
	typedef vec2_packed<std::uint64_t> ulvec2_packed;
	typedef vec4_packed<std::int64_t> lvec4_packed;
	typedef vec3_packed<std::int64_t> lvec3_packed;
	typedef vec2_packed<std::int64_t> lvec2_packed;
	typedef vec4_packed<std::uint32_t> uivec4_packed;
	typedef vec3_packed<std::uint32_t> uivec3_packed;
	typedef vec2_packed<std::uint32_t> uivec2_packed;
	typedef vec4_packed<std::int32_t> ivec4_packed;
	typedef vec3_packed<std::int32_t> ivec3_packed;
	typedef vec2_packed<std::int32_t> ivec2_packed;
}	 // namespace sek::math