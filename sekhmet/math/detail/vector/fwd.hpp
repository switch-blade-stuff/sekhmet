/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../util.hpp"

namespace sek::math
{
	enum class storage_policy : int
	{
		/** @brief Values are stored with potential over-alignment to allow for SIMD optimizations. */
		OPTIMAL,
		/** @brief Values are tightly packed in memory.
		 * @note Packed storage is not SIMD-optimized. */
		PACKED
	};

	template<typename T, std::size_t N, storage_policy Policy>
	class basic_vec;
	template<typename...>
	class vec_mask;
}