//
// Created by switchblade on 2021-12-16.
//

#pragma once

#include "../util.hpp"
#include "../vector_data.hpp"

namespace sek::math::detail
{
	template<typename T, std::size_t N>
	constexpr void vector_add(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = lhs[i] + rhs[i];
	}
	template<typename T, std::size_t N>
	constexpr void vector_sub(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = lhs[i] - rhs[i];
	}

	template<typename T, std::size_t N>
	constexpr void vector_mul(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, T rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = lhs[i] * rhs;
	}
	template<typename T, std::size_t N>
	constexpr void vector_div(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, T rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = lhs[i] / rhs;
	}

	template<typename T, std::size_t N>
	constexpr void vector_and(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = lhs[i] & rhs[i];
	}
	template<typename T, std::size_t N>
	constexpr void vector_or(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = lhs[i] | rhs[i];
	}
	template<typename T, std::size_t N>
	constexpr void vector_xor(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = lhs[i] ^ rhs[i];
	}
	template<typename T, std::size_t N>
	constexpr void vector_inv(vector_data_t<T, N> &out, vector_data_t<T, N> data) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = ~data[i];
	}

	template<typename T, std::size_t N>
	constexpr void vector_neg(vector_data_t<T, N> &out, vector_data_t<T, N> data) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = -data[i];
	}
	template<typename T, std::size_t N>
	constexpr void vector_abs(vector_data_t<T, N> &out, vector_data_t<T, N> data) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = std::abs(data[i]);
	}

	template<typename T, std::size_t N>
	constexpr void vector_max(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = max(lhs[i], rhs[i]);
	}
	template<typename T, std::size_t N>
	constexpr void vector_min(vector_data_t<T, N> &out, vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = min(lhs[i], rhs[i]);
	}

	template<typename T, std::size_t N>
	constexpr T vector_dot(vector_data_t<T, N> lhs, vector_data_t<T, N> rhs) noexcept
	{
		T result = {};
		for (std::size_t i = 0; i < N; i++) result += lhs[i] * rhs[i];
		return result;
	}

	template<typename T, std::size_t N>
	constexpr void vector_sqrt(vector_data_t<T, N> &out, vector_data_t<T, N> data) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = std::sqrt(data[i]);
	}
	template<typename T, std::size_t N>
	constexpr void vector_rsqrt(vector_data_t<T, N> &out, vector_data_t<T, N> data) noexcept
	{
		for (std::size_t i = 0; i < N; i++) out[i] = static_cast<T>(1) / std::sqrt(data[i]);
	}

	template<typename T, std::size_t N>
	constexpr void vector_norm(vector_data_t<T, N> &out, vector_data_t<T, N> data) noexcept
	{
		vector_div(out, std::sqrt(vector_dot(data, data)));
	}
}	 // namespace sek::math::detail