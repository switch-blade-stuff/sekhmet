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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include "../util.hpp"
#include "../vector_data.hpp"

namespace sek::math::detail
{
	/* Inline namespace is used to always prefer the generic implementation in constant evaluated contexts. */
	inline namespace generic
	{
		template<std::size_t I, std::size_t N, typename F, typename... Args>
		constexpr void vector_unwrap(F &&f, Args &&...args)
		{
			if constexpr (I != N)
			{
				f(I, std::forward<Args>(args)...);
				vector_unwrap<I + 1, N>(std::forward<F>(f), std::forward<Args>(args)...);
			}
		}
		template<std::size_t N, typename F, typename... Args>
		constexpr void vector_unwrap(F &&f, Args &&...args)
		{
			if constexpr (N <= 4)
				vector_unwrap<0, N>(std::forward<F>(f), std::forward<Args>(args)...);
			else
				for (std::size_t i = 0; i < N; ++i) f(i, std::forward<Args>(args)...);
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_add(vector_data<T, N, UseSimd> &out,
								  const vector_data<T, N, UseSimd> &l,
								  const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] + r[i]; });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_sub(vector_data<T, N, UseSimd> &out,
								  const vector_data<T, N, UseSimd> &l,
								  const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] - r[i]; });
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_mul(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l, T r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] * r; });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_div(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l, T r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] / r; });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_div(vector_data<T, N, UseSimd> &out, T l, const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l / r[i]; });
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_and(vector_data<T, N, UseSimd> &out,
								  const vector_data<T, N, UseSimd> &l,
								  const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] & r[i]; });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_or(vector_data<T, N, UseSimd> &out,
								 const vector_data<T, N, UseSimd> &l,
								 const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] | r[i]; });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_xor(vector_data<T, N, UseSimd> &out,
								  const vector_data<T, N, UseSimd> &l,
								  const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] ^ r[i]; });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_inv(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = ~l[i]; });
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_neg(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = -l[i]; });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_abs(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::abs(l[i]); });
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_max(vector_data<T, N, UseSimd> &out,
								  const vector_data<T, N, UseSimd> &l,
								  const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = max(l[i], r[i]); });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_min(vector_data<T, N, UseSimd> &out,
								  const vector_data<T, N, UseSimd> &l,
								  const vector_data<T, N, UseSimd> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = min(l[i], r[i]); });
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr T vector_dot(const vector_data<T, N, UseSimd> &l, const vector_data<T, N, UseSimd> &r) noexcept
		{
			T result = {};
			vector_unwrap<N>([&](auto i) { result += l[i] * r[i]; });
			return result;
		}
		template<typename T, bool UseSimd>
		constexpr void vector_cross(vector_data<T, 3, UseSimd> &out,
									const vector_data<T, 3, UseSimd> &l,
									const vector_data<T, 3, UseSimd> &r) noexcept
		{
			out[0] = l[1] * r[2] - l[2] * r[1];
			out[1] = l[2] * r[0] - l[0] * r[2];
			out[2] = l[0] * r[1] - l[1] * r[0];
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_sqrt(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::sqrt(l[i]); });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_rsqrt(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(1) / std::sqrt(l[i]); });
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_norm(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_div(out, l, std::sqrt(vector_dot(l, l)));
		}

		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_rad(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = rad(l[i]); });
		}
		template<typename T, std::size_t N, bool UseSimd>
		constexpr void vector_deg(vector_data<T, N, UseSimd> &out, const vector_data<T, N, UseSimd> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = deg(l[i]); });
		}

		template<std::size_t J, typename T, std::size_t N, bool Simd1, std::size_t M, bool Simd2, std::size_t I, std::size_t... Is>
		constexpr void shuffle_unwrap(vector_data<T, N, Simd1> &out,
									  const vector_data<T, M, Simd2> &l,
									  std::index_sequence<I, Is...>) noexcept
		{
			out[J] = l[I];
			if constexpr (sizeof...(Is) != 0) shuffle_unwrap<J + 1>(out, l, std::index_sequence<Is...>{});
		}
		template<typename T, std::size_t N, bool Simd1, std::size_t M, bool Simd2, std::size_t... Is>
		constexpr void vector_shuffle(vector_data<T, N, Simd1> &out,
									  const vector_data<T, M, Simd2> &l,
									  std::index_sequence<Is...> s) noexcept
		{
			shuffle_unwrap<0>(out, l, s);
		}
	}	 // namespace generic
}	 // namespace sek::math::detail