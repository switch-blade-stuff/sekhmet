//
// Created by switchblade on 2021-12-16.
//

#pragma once

#include "../util.hpp"
#include "../vector_data.hpp"

namespace sek::math::detail
{
	/* Inline namespace is used to always prefer the generic implementation in constant evaluated contexts. */
	inline namespace generic
	{
		template<typename T, std::size_t N>
		constexpr void vector_add(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l[i] + r[i];
		}
		template<typename T, std::size_t N>
		constexpr void vector_sub(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l[i] - r[i];
		}

		template<typename T, std::size_t N>
		constexpr void vector_mul(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, T r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l[i] * r;
		}
		template<typename T, std::size_t N>
		constexpr void vector_div(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, T r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l[i] / r;
		}
		template<typename T, std::size_t N>
		constexpr void vector_div(vector_data_t<T, N> &out, T l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l / r[i];
		}

		template<typename T, std::size_t N>
		constexpr void vector_and(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l[i] & r[i];
		}
		template<typename T, std::size_t N>
		constexpr void vector_or(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l[i] | r[i];
		}
		template<typename T, std::size_t N>
		constexpr void vector_xor(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = l[i] ^ r[i];
		}
		template<typename T, std::size_t N>
		constexpr void vector_inv(vector_data_t<T, N> &out, const vector_data_t<T, N> &l) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = ~l[i];
		}

		template<typename T, std::size_t N>
		constexpr void vector_neg(vector_data_t<T, N> &out, const vector_data_t<T, N> &l) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = -l[i];
		}
		template<typename T, std::size_t N>
		constexpr void vector_abs(vector_data_t<T, N> &out, const vector_data_t<T, N> &l) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = std::abs(l[i]);
		}

		template<typename T, std::size_t N>
		constexpr void vector_max(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = max(l[i], r[i]);
		}
		template<typename T, std::size_t N>
		constexpr void vector_min(vector_data_t<T, N> &out, const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = min(l[i], r[i]);
		}

		template<typename T, std::size_t N>
		constexpr T vector_dot(const vector_data_t<T, N> &l, const vector_data_t<T, N> &r) noexcept
		{
			T result = {};
			for (std::size_t i = 0; i < N; i++) result += l[i] * r[i];
			return result;
		}
		template<typename T>
		constexpr void vector_cross(vector_data_t<T, 3> &out, const vector_data_t<T, 3> &l, const vector_data_t<T, 3> &r) noexcept
		{
			out[0] = l[1] * r[2] - l[2] * r[1];
			out[1] = l[2] * r[0] - l[0] * r[2];
			out[2] = l[0] * r[1] - l[1] * r[0];
		}

		template<typename T, std::size_t N>
		constexpr void vector_sqrt(vector_data_t<T, N> &out, const vector_data_t<T, N> &l) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = std::sqrt(l[i]);
		}
		template<typename T, std::size_t N>
		constexpr void vector_rsqrt(vector_data_t<T, N> &out, const vector_data_t<T, N> &l) noexcept
		{
			for (std::size_t i = 0; i < N; i++) out[i] = static_cast<T>(1) / std::sqrt(l[i]);
		}

		template<typename T, std::size_t N>
		constexpr void vector_norm(vector_data_t<T, N> &out, const vector_data_t<T, N> &l) noexcept
		{
			vector_div(out, l, std::sqrt(vector_dot(l, l)));
		}

		template<std::size_t J, typename T, std::size_t N, std::size_t M, std::size_t I, std::size_t... Is>
		constexpr void shuffle_unwrap(vector_data_t<T, N> &out, const vector_data_t<T, M> &l, std::index_sequence<I, Is...>) noexcept
		{
			out[J] = l[I];
			if constexpr (sizeof...(Is) != 0) shuffle_unwrap<J + 1>(out, l, std::index_sequence<Is...>{});
		}
		template<typename T, std::size_t N, std::size_t M, std::size_t... Is>
		constexpr void vector_shuffle(vector_data_t<T, N> &out, const vector_data_t<T, M> &l, std::index_sequence<Is...> s) noexcept
		{
			shuffle_unwrap<0>(out, l, s);
		}
	}	 // namespace generic
}	 // namespace sek::math::detail