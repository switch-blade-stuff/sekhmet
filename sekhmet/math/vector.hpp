/*
 * Created by switchblade on 2021-12-29
 */

#pragma once

#include "detail/vector.hpp"

namespace sek
{
	template<typename T = float>
	using vec4 = basic_vec<T, 4>;
	template<typename T = float>
	using vec3 = basic_vec<T, 3>;
	template<typename T = float>
	using vec2 = basic_vec<T, 2>;
	template<typename T = float>
	using vec4_packed = basic_vec<T, 4, policy_t::DEFAULT_PACKED>;
	template<typename T = float>
	using vec3_packed = basic_vec<T, 3, policy_t::DEFAULT_PACKED>;
	template<typename T = float>
	using vec2_packed = basic_vec<T, 2, policy_t::DEFAULT_PACKED>;

	typedef vec4<double> vec4d;
	typedef vec3<double> vec3d;
	typedef vec2<double> vec2d;
	typedef vec4<float> vec4f;
	typedef vec3<float> vec3f;
	typedef vec2<float> vec2f;
	typedef vec4<std::uint64_t> vec4ul;
	typedef vec3<std::uint64_t> vec3ul;
	typedef vec2<std::uint64_t> vec2ul;
	typedef vec4<std::int64_t> vec4l;
	typedef vec3<std::int64_t> vec3l;
	typedef vec2<std::int64_t> vec2l;
	typedef vec4<std::uint32_t> vec4ui;
	typedef vec3<std::uint32_t> vec3ui;
	typedef vec2<std::uint32_t> vec2ui;
	typedef vec4<std::int32_t> vec4i;
	typedef vec3<std::int32_t> vec3i;
	typedef vec2<std::int32_t> vec2i;
	typedef vec4_packed<double> vec4d_packed;
	typedef vec3_packed<double> vec3d_packed;
	typedef vec2_packed<double> vec2d_packed;
	typedef vec4_packed<float> vec4f_packed;
	typedef vec3_packed<float> vec3f_packed;
	typedef vec2_packed<float> vec2f_packed;
	typedef vec4_packed<std::uint64_t> vec4ul_packed;
	typedef vec3_packed<std::uint64_t> vec3ul_packed;
	typedef vec2_packed<std::uint64_t> vec2ul_packed;
	typedef vec4_packed<std::int64_t> vec4l_packed;
	typedef vec3_packed<std::int64_t> vec3l_packed;
	typedef vec2_packed<std::int64_t> vec2l_packed;
	typedef vec4_packed<std::uint32_t> vec4ui_packed;
	typedef vec3_packed<std::uint32_t> vec3ui_packed;
	typedef vec2_packed<std::uint32_t> vec2ui_packed;
	typedef vec4_packed<std::int32_t> vec4i_packed;
	typedef vec3_packed<std::int32_t> vec3i_packed;
	typedef vec2_packed<std::int32_t> vec2i_packed;

	template<typename T = float>
	using vec4_mask = vec_mask<basic_vec<T, 4>>;
	template<typename T = float>
	using vec3_mask = vec_mask<basic_vec<T, 3>>;
	template<typename T = float>
	using vec2_mask = vec_mask<basic_vec<T, 2>>;
	template<typename T = float>
	using vec4_mask_packed = vec_mask<basic_vec<T, 4, policy_t::DEFAULT_PACKED>>;
	template<typename T = float>
	using vec3_mask_packed = vec_mask<basic_vec<T, 3, policy_t::DEFAULT_PACKED>>;
	template<typename T = float>
	using vec2_mask_packed = vec_mask<basic_vec<T, 2, policy_t::DEFAULT_PACKED>>;

	typedef vec4_mask<double> vec4d_mask;
	typedef vec3_mask<double> vec3d_mask;
	typedef vec2_mask<double> vec2d_mask;
	typedef vec4_mask<float> vec4f_mask;
	typedef vec3_mask<float> vec3f_mask;
	typedef vec2_mask<float> vec2f_mask;
	typedef vec4_mask<std::uint64_t> vec4ul_mask;
	typedef vec3_mask<std::uint64_t> vec3ul_mask;
	typedef vec2_mask<std::uint64_t> vec2ul_mask;
	typedef vec4_mask<std::int64_t> vec4l_mask;
	typedef vec3_mask<std::int64_t> vec3l_mask;
	typedef vec2_mask<std::int64_t> vec2l_mask;
	typedef vec4_mask<std::uint32_t> vec4ui_mask;
	typedef vec3_mask<std::uint32_t> vec3ui_mask;
	typedef vec2_mask<std::uint32_t> vec2ui_mask;
	typedef vec4_mask<std::int32_t> vec4i_mask;
	typedef vec3_mask<std::int32_t> vec3i_mask;
	typedef vec2_mask<std::int32_t> vec2i_mask;
	typedef vec4_mask_packed<double> vec4d_mask_packed;
	typedef vec3_mask_packed<double> vec3d_mask_packed;
	typedef vec2_mask_packed<double> vec2d_mask_packed;
	typedef vec4_mask_packed<float> vec4f_mask_packed;
	typedef vec3_mask_packed<float> vec3f_mask_packed;
	typedef vec2_mask_packed<float> vec2f_mask_packed;
	typedef vec4_mask_packed<std::uint64_t> vec4ul_mask_packed;
	typedef vec3_mask_packed<std::uint64_t> vec3ul_mask_packed;
	typedef vec2_mask_packed<std::uint64_t> vec2ul_mask_packed;
	typedef vec4_mask_packed<std::int64_t> vec4l_mask_packed;
	typedef vec3_mask_packed<std::int64_t> vec3l_mask_packed;
	typedef vec2_mask_packed<std::int64_t> vec2l_mask_packed;
	typedef vec4_mask_packed<std::uint32_t> vec4ui_mask_packed;
	typedef vec3_mask_packed<std::uint32_t> vec3ui_mask_packed;
	typedef vec2_mask_packed<std::uint32_t> vec2ui_mask_packed;
	typedef vec4_mask_packed<std::int32_t> vec4i_mask_packed;
	typedef vec3_mask_packed<std::int32_t> vec3i_mask_packed;
	typedef vec2_mask_packed<std::int32_t> vec2i_mask_packed;
}	 // namespace sek