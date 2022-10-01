/*
 * Created by switchblade on 2022-03-14
 */

#pragma once

#include "detail/sysrandom.hpp"
#include "detail/xoroshiro.hpp"

namespace sek
{
	/** 256-bit version of xoroshiro. */
	template<typename T>
	using xoroshiro256 = xoroshiro<T, 256>;
	/** 128-bit version of xoroshiro. */
	template<typename T>
	using xoroshiro128 = xoroshiro<T, 128>;
}	 // namespace sek