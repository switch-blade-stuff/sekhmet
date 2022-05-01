//
// Created by switchblade on 2022-01-31.
//

#pragma once

#include "../vector_data.hpp"
#include "avx.hpp"
#include "sse.hpp"
#include <type_traits>

#define SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(func, ...) requires(requires { func(__VA_ARGS__); })

namespace sek::math::detail
{
	template<typename T, std::size_t N>
	inline void vector_add(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_add, out, lhs, rhs)
	{

		x86_simd_add(out, lhs, rhs);
	}
	template<typename T, std::size_t N>
	inline void vector_sub(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_sub, out, lhs, rhs)
	{
		x86_simd_sub(out, lhs, rhs);
	}

	template<typename T, std::size_t N>
	inline void vector_mul(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, T rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_mul_s, out, lhs, rhs)
	{
		x86_simd_mul_s(out, lhs, rhs);
	}
	template<typename T, std::size_t N>
	inline void vector_div(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, T rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_div_s, out, lhs, rhs)
	{
		x86_simd_div_s(out, lhs, rhs);
	}

	template<typename T, std::size_t N>
	inline void vector_neg(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_neg, out, data)
	{
		x86_simd_neg(out, data);
	}
	template<typename T, std::size_t N>
	inline void vector_abs(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_abs, out, data)
	{
		x86_simd_abs(out, data);
	}

	template<typename T, std::size_t N>
	inline void vector_max(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_max, out, lhs, rhs)
	{
		x86_simd_max(out, lhs, rhs);
	}
	template<typename T, std::size_t N>
	inline void vector_min(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_min, out, lhs, rhs)
	{
		x86_simd_min(out, lhs, rhs);
	}

	template<typename T, std::size_t N>
	inline void vector_sqrt(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_sqrt, out, data)
	{
		x86_simd_sqrt(out, data);
	}
	template<typename T, std::size_t N>
	inline void vector_rsqrt(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_rsqrt, out, data)
	{
		x86_simd_rsqrt(out, data);
	}

	template<typename T, std::size_t N>
	inline void vector_and(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_and, out, lhs, rhs)
	{
		x86_simd_and(out, lhs, rhs);
	}
	template<typename T, std::size_t N>
	inline void vector_or(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_or, out, lhs, rhs)
	{
		x86_simd_or(out, lhs, rhs);
	}
	template<typename T, std::size_t N>
	inline void vector_xor(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_xor, out, lhs, rhs)
	{
		x86_simd_xor(out, lhs, rhs);
	}
	template<typename T, std::size_t N>
	inline void vector_inv(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_inv, out, data)
	{
		x86_simd_inv(out, data);
	}

	template<typename T, std::size_t N>
	inline T vector_dot(vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_dot, lhs, rhs)
	{
		return x86_simd_dot(lhs, rhs);
	}

	template<typename T, std::size_t N>
	inline void vector_norm(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_norm, out, data)
	{
		x86_simd_norm(out, data);
	}
}	 // namespace sek::math::detail
