//
// Created by switchblade on 30/04/22.
//

#pragma once

#include "util.hpp"

namespace sek::math::detail
{
	template<typename, std::size_t>
	struct simd_data;

	template<typename T, std::size_t N, typename = void>
	struct simd_data_defined : std::false_type
	{
	};
	template<typename T, std::size_t N>
	struct simd_data_defined<T, N, std::void_t<decltype(sizeof(simd_data<T, N>))>> : std::true_type
	{
	};
	template<typename T, std::size_t N>
	concept simd_data_exists = (simd_data_defined<T, N>::value && !std::is_empty_v<simd_data<T, N>>);

	template<typename T, std::size_t N, std::size_t I>
	struct simd_data_selector
	{
		using type = simd_data<T, I>[N == I ? 1 : align(sizeof(T[N]), sizeof(simd_data<T, I>)) / sizeof(simd_data<T, I>)];
	};
	template<typename T, std::size_t N>
	struct simd_data_selector<T, N, 1>
	{
	};
	template<typename T, std::size_t N, std::size_t I>
		requires(!simd_data_exists<T, I> || sizeof(T[N]) * 2 <= sizeof(simd_data<T, I>))
	struct simd_data_selector<T, N, I> : simd_data_selector<T, N, I / 2>
	{
	};

	/* Powers of 2 are used, since most SIMD architectures have power of 2 register sizes. */
	template<typename T, std::size_t N>
	using simd_data_t = typename simd_data_selector<T, N, next_pow_2(N)>::type;
	template<typename T, std::size_t N>
	concept has_simd_data = requires { typename simd_data_selector<T, N, next_pow_2(N)>::type; };
}	 // namespace sek::math::detail
