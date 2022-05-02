//
// Created by switchblade on 2022-01-31.
//

#pragma once

#include "../vector_data.hpp"
#include "avx.hpp"
#include "common.hpp"
#include "sse.hpp"
#include <type_traits>

namespace sek::math::detail
{
	template<typename T, std::size_t N>
	inline void vector_add(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_add, out.simd, l.simd, r.simd)
	{
		x86_simd_add(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_sub(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_sub, out.simd, l.simd, r.simd)
	{
		x86_simd_sub(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_mul(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, T r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_mul_s, out.simd, l.simd, r)
	{
		x86_simd_mul_s(out.simd, l.simd, r);
	}
	template<typename T, std::size_t N>
	inline void vector_div(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, T r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_div_s, out.simd, l.simd, r)
	{
		x86_simd_div_s(out.simd, l.simd, r);
	}
	template<typename T, std::size_t N>
	inline void vector_neg(vector_data<T, N, true> &out, const vector_data<T, N, true> &l) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_neg, out.simd, l.simd)
	{
		x86_simd_neg(out.simd, l.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_abs(vector_data<T, N, true> &out, const vector_data<T, N, true> &l) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_abs, out.simd, l.simd)
	{
		x86_simd_abs(out.simd, l.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_max(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_max, out.simd, l.simd, r.simd)
	{
		x86_simd_max(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_min(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_min, out.simd, l.simd, r.simd)
	{
		x86_simd_min(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_sqrt(vector_data<T, N, true> &out, const vector_data<T, N, true> &l) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_sqrt, out.simd, l.simd)
	{
		x86_simd_sqrt(out.simd, l.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_rsqrt(vector_data<T, N, true> &out, const vector_data<T, N, true> &l) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_rsqrt, out.simd, l.simd)
	{
		x86_simd_rsqrt(out.simd, l.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_and(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_and, out.simd, l.simd, r.simd)
	{
		x86_simd_and(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_or(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_or, out.simd, l.simd, r.simd)
	{
		x86_simd_or(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_xor(vector_data<T, N, true> &out, const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_xor, out.simd, l.simd, r.simd)
	{
		x86_simd_xor(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_inv(vector_data<T, N, true> &out, const vector_data<T, N, true> &l) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_inv, out.simd, l.simd)
	{
		x86_simd_inv(out.simd, l.simd);
	}
	template<typename T, std::size_t N>
	inline T vector_dot(const vector_data<T, N, true> &l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_dot, l.simd, r.simd)
	{
		return x86_simd_dot(l.simd, r.simd);
	}
	template<typename T>
	inline void vector_cross(vector_data<T, 3, true> &out, const vector_data<T, 3, true> &l, const vector_data<T, 3, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_cross, out.simd, l.simd, r.simd)
	{
		x86_simd_cross(out.simd, l.simd, r.simd);
	}
	template<typename T, std::size_t N>
	inline void vector_norm(vector_data<T, N, true> &out, const vector_data<T, N, true> &l) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_norm, out.simd, l.simd)
	{
		x86_simd_norm(out.simd, l.simd);
	}
}	 // namespace sek::math::detail
