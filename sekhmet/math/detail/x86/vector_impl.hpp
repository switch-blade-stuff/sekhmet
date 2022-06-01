/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2022-01-31
 */

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
	inline void vector_div(vector_data<T, N, true> &out, T l, const vector_data<T, N, true> &r) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_div_s, out.simd, l, r.simd)
	{
		x86_simd_div_s(out.simd, l, r.simd);
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

	template<typename T, std::size_t N, std::size_t M, std::size_t... Is>
	inline void vector_shuffle(vector_data<T, N, true> &out, const vector_data<T, M, true> &l, std::index_sequence<Is...> s) noexcept
		SEK_REQUIRES_OVERLOAD(x86_simd_shuffle, out.simd, l.simd, s)
	{
		x86_simd_shuffle(out.simd, l.simd, s);
	}
}	 // namespace sek::math::detail
