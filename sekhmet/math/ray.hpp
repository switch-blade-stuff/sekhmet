//
// Created by switch_blade on 2022-10-01.
//

#pragma once

#include "detail/vector.hpp"

namespace sek
{
	/** @brief Structure representing a ray.
	 * @tparam T Value type of the ray's origin and direction vectors.
	 * @tparam N Dimension of the ray's origin and direction vectors.
	 * @tparam Policy Policy used for storage & optimization. */
	template<typename T, std::size_t N, policy_t Policy = policy_t::DEFAULT>
	struct basic_ray
	{
		typedef basic_vec<T, N> vec_type;

		vec_type origin;
		vec_type direction;
	};

	template<typename T = float>
	using ray4 = basic_ray<T, 4>;
	template<typename T = float>
	using ray3 = basic_ray<T, 3>;
	template<typename T = float>
	using ray2 = basic_ray<T, 2>;
	template<typename T = float>
	using ray4_packed = basic_ray<T, 4, policy_t::DEFAULT_PACKED>;
	template<typename T = float>
	using ray3_packed = basic_ray<T, 3, policy_t::DEFAULT_PACKED>;
	template<typename T = float>
	using ray2_packed = basic_ray<T, 2, policy_t::DEFAULT_PACKED>;

	typedef ray4<double> ray4d;
	typedef ray3<double> ray3d;
	typedef ray2<double> ray2d;
	typedef ray4<float> ray4f;
	typedef ray3<float> ray3f;
	typedef ray2<float> ray2f;
	typedef ray4<std::uint64_t> ray4ul;
	typedef ray3<std::uint64_t> ray3ul;
	typedef ray2<std::uint64_t> ray2ul;
	typedef ray4<std::int64_t> ray4l;
	typedef ray3<std::int64_t> ray3l;
	typedef ray2<std::int64_t> ray2l;
	typedef ray4<std::uint32_t> ray4ui;
	typedef ray3<std::uint32_t> ray3ui;
	typedef ray2<std::uint32_t> ray2ui;
	typedef ray4<std::int32_t> ray4i;
	typedef ray3<std::int32_t> ray3i;
	typedef ray2<std::int32_t> ray2i;
	typedef ray4_packed<double> ray4d_packed;
	typedef ray3_packed<double> ray3d_packed;
	typedef ray2_packed<double> ray2d_packed;
	typedef ray4_packed<float> ray4f_packed;
	typedef ray3_packed<float> ray3f_packed;
	typedef ray2_packed<float> ray2f_packed;
	typedef ray4_packed<std::uint64_t> ray4ul_packed;
	typedef ray3_packed<std::uint64_t> ray3ul_packed;
	typedef ray2_packed<std::uint64_t> ray2ul_packed;
	typedef ray4_packed<std::int64_t> ray4l_packed;
	typedef ray3_packed<std::int64_t> ray3l_packed;
	typedef ray2_packed<std::int64_t> ray2l_packed;
	typedef ray4_packed<std::uint32_t> ray4ui_packed;
	typedef ray3_packed<std::uint32_t> ray3ui_packed;
	typedef ray2_packed<std::uint32_t> ray2ui_packed;
	typedef ray4_packed<std::int32_t> ray4i_packed;
	typedef ray3_packed<std::int32_t> ray3i_packed;
	typedef ray2_packed<std::int32_t> ray2i_packed;
}	 // namespace sek