//
// Created by switchblade on 2022-01-31.
//

#pragma once

#include "../vector_data.hpp"
#include "avx.hpp"
#include "simd_util.hpp"
#include "sse.hpp"
#include <type_traits>

#define SEK_DETAIL_SIMD_ARG(storage) storage.data
#define SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(func, ...) requires(requires { func(__VA_ARGS__); })

namespace sek::math::detail
{
	template<typename T, std::size_t N>
	constexpr void vector_add(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_add, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		x86_simd_add(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}
	template<typename T, std::size_t N>
	constexpr void vector_sub(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_sub, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		x86_simd_sub(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}

	template<typename T, std::size_t N>
	constexpr void vector_mul(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, T rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_mul_s, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), rhs)
	{
		x86_simd_mul_s(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), rhs);
	}
	template<typename T, std::size_t N>
	constexpr void vector_div(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, T rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_div_s, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), rhs)
	{
		x86_simd_div_s(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), rhs);
	}

	template<typename T, std::size_t N>
	constexpr void vector_neg(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_neg, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data))
	{
		x86_simd_neg(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data));
	}
	template<typename T, std::size_t N>
	constexpr void vector_abs(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_abs, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data))
	{
		x86_simd_abs(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data));
	}

	template<typename T, std::size_t N>
	constexpr void vector_max(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_max, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		x86_simd_max(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}
	template<typename T, std::size_t N>
	constexpr void vector_min(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_min, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		x86_simd_min(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}

	template<typename T, std::size_t N>
	constexpr void vector_sqrt(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_sqrt, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data))
	{
		x86_simd_sqrt(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data));
	}
	template<typename T, std::size_t N>
	constexpr void vector_rsqrt(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_rsqrt, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data))
	{
		x86_simd_rsqrt(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data));
	}

	template<typename T, std::size_t N>
	constexpr void vector_and(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_and, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		x86_simd_and(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}
	template<typename T, std::size_t N>
	constexpr void vector_or(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_or, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		x86_simd_or(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}
	template<typename T, std::size_t N>
	constexpr void vector_xor(vector_data<T, N, true> &out, vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_xor, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		x86_simd_xor(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}
	template<typename T, std::size_t N>
	constexpr void vector_inv(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_inv, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data))
	{
		x86_simd_inv(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data));
	}

	template<typename T, std::size_t N>
	constexpr T vector_dot(vector_data<T, N, true> lhs, vector_data<T, N, true> rhs) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_dot, SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs))
	{
		return x86_simd_dot(SEK_DETAIL_SIMD_ARG(lhs), SEK_DETAIL_SIMD_ARG(rhs));
	}

	template<typename T, std::size_t N>
	constexpr void vector_norm(vector_data<T, N, true> &out, vector_data<T, N, true> data) noexcept
		SEK_DETAIL_REQUIRE_SIMD_OVERLOAD(x86_simd_norm, SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data))
	{
		x86_simd_norm(SEK_DETAIL_SIMD_ARG(out), SEK_DETAIL_SIMD_ARG(data));
	}
}	 // namespace sek::math::detail
