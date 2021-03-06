/*
 * Created by switchblade on 07/07/22
 */

#pragma once

namespace sek::math
{
	enum policy_t : int
	{
		/** Precision of mathematical operations is prioritized over speed. */
		HIGHP = 0b0,
		/** Qeed of mathematical operations is prioritized over precision. */
		FAST = 0b1,

		PRECISION_MASK = 0b1,

		/** Elements are tightly packed in memory to take up as little space as possible. */
		PACKED = 0b00,
		/** Elements are over-aligned to allow for SIMD optimizations. */
		ALIGNED = 0b10,

		STORAGE_MASK = 0b10,

		/** Simd-enabled policy with priority for speed. Equivalent to `ALIGNED | FAST`. */
		FAST_SIMD = FAST | ALIGNED,
		/** Packed (non-SIMD) policy with priority for speed. Equivalent to `PACKED | FAST`. */
		FAST_PACKED = FAST | ALIGNED,

		/** Default SIMD-enabled policy. Equivalent to `FAST_SIMD`. */
		DEFAULT = FAST_SIMD,
		/** Default non-SIMD policy. Equivalent to `FAST_PACKED`. */
		DEFAULT_PACKED = FAST_PACKED,
	};
}	 // namespace sek::math