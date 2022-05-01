//
// Created by switchblade on 30/04/22.
//

#pragma once

#include "util.hpp"

namespace sek::math::detail
{
	template<typename, std::size_t>
	struct simd_t;

	template<typename T, std::size_t N, typename = void>
	struct simd_defined : std::false_type
	{
	};
	template<typename T, std::size_t N>
	struct simd_defined<T, N, std::void_t<decltype(sizeof(simd_t<T, N>))>> : std::true_type
	{
	};
	template<typename T, std::size_t N>
	concept simd_exists = (simd_defined<T, N>::value && !std::is_empty_v<simd_t<T, N>>);
}	 // namespace sek::math::detail
