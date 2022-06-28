/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../util.hpp"

namespace sek::math
{
	enum storage_policy : int
	{
		/** Elements are stored with potential over-alignment to allow for SIMD optimizations.
		 * Precision of certain mathematical operations may be sacrificed for speed. */
		SPEED,
		/** Elements are stored with potential over-alignment to allow for SIMD optimizations.
		 * Accuracy of mathematical operations is preferred over speed. */
		PRECISION,
		/** Elements are tightly packed in memory. SIMD optimizations may not be possible due to alignment requirements. */
		SIZE
	};

	template<typename T, std::size_t N, storage_policy Policy>
	class basic_vec;
	template<typename...>
	class vec_mask;
}	 // namespace sek::math