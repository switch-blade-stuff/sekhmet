/*
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include "../storage.hpp"
#include "../util.hpp"

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
			vector_unwrap<0, N>(std::forward<F>(f), std::forward<Args>(args)...);
		}

		template<std::size_t J, typename T, std::size_t N, std::size_t M, storage_policy P1, storage_policy P2, std::size_t I, std::size_t... Is>
		constexpr void shuffle_unwrap(vector_data<T, N, P1> &out, const vector_data<T, M, P2> &l, std::index_sequence<I, Is...>) noexcept
		{
			out[J] = l[I];
			if constexpr (sizeof...(Is) != 0) shuffle_unwrap<J + 1>(out, l, std::index_sequence<Is...>{});
		}
		template<typename T, std::size_t N, std::size_t M, storage_policy P1, storage_policy P2, std::size_t... Is>
		constexpr void vector_shuffle(vector_data<T, N, P1> &out, const vector_data<T, M, P2> &l, std::index_sequence<Is...> s) noexcept
		{
			shuffle_unwrap<0>(out, l, s);
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_interleave(vector_data<T, N, P> &out,
										 const vector_data<T, N, P> &l,
										 const vector_data<T, N, P> &r,
										 const mask_data<T, N, P> &m) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = m[i] ? l[i] : r[i]; });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_add(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] + r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_sub(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] - r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_mul(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] * r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_div(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] / r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_mod(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] % r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_fmod(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::fmod(l[i], r[i])); });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_exp(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::exp(l[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_exp2(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::exp2(l[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_expm1(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::expm1(l[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_log(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::log(l[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_log10(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::log10(l[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_log2(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::log2(l[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_log1p(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = std::log1p(l[i]); });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_pow(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::pow(l[i], r[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_sqrt(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::sqrt(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_cbrt(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::cbrt(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_rsqrt(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(1) / static_cast<T>(std::sqrt(l[i])); });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_and(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] & r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_or(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] | r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_xor(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = l[i] ^ r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_inv(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = ~l[i]; });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_neg(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = -l[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_abs(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::abs(l[i])); });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_max(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = max(l[i], r[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_min(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = min(l[i], r[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_round(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::round(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_floor(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::floor(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_ceil(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::ceil(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_trunc(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::trunc(l[i])); });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr T vector_dot(const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			T result = {};
			vector_unwrap<N>([&](auto i) { result += l[i] * r[i]; });
			return result;
		}
		template<typename T, storage_policy P>
		constexpr void vector_cross(vector_data<T, 3, P> &out, const vector_data<T, 3, P> &l, const vector_data<T, 3, P> &r) noexcept
		{
			out[0] = l[1] * r[2] - l[2] * r[1];
			out[1] = l[2] * r[0] - l[0] * r[2];
			out[2] = l[0] * r[1] - l[1] * r[0];
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_norm(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			const auto r = static_cast<T>(std::sqrt(vector_dot(l, l)));
			vector_unwrap<N>([&](auto i) { out[i] = l[i] / r; });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_sin(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::sin(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_cos(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::cos(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_tan(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::tan(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_asin(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::asin(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_acos(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::acos(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_atan(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::atan(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_sinh(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::sinh(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_cosh(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::cosh(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_tanh(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::tanh(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_asinh(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::asinh(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_acosh(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::acosh(l[i])); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_atanh(vector_data<T, N, P> &out, const vector_data<T, N, P> &l) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = static_cast<T>(std::atanh(l[i])); });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_cmp(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r, auto p) noexcept
		{
			vector_unwrap<N>([&](auto i) { out[i] = p(l[i], r[i]); });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_eq(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_cmp(out, l, r, [](T a, T b) { return a == b; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_ne(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_cmp(out, l, r, [](T a, T b) { return a != b; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_lt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_cmp(out, l, r, [](T a, T b) { return a < b; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_le(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_cmp(out, l, r, [](T a, T b) { return a <= b; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_gt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_cmp(out, l, r, [](T a, T b) { return a > b; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void vector_ge(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		{
			vector_cmp(out, l, r, [](T a, T b) { return a >= b; });
		}
	}	 // namespace generic
}	 // namespace sek::math::detail