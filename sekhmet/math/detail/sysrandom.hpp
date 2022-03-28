//
// Created by switchblade on 2022-03-14.
//

#pragma once

#include "../../detail/define.h"

namespace sek::math::detail
{
	/** Fills a buffer with random bytes using OS-specific method.
	 * @param dest Destination buffer.
	 * @param len Length of the destination buffer.
	 * @return Amount of bytes filled, or -1 if a failure occurred. */
	SEK_API std::size_t sys_random(void *dest, std::size_t len) noexcept;
}     // namespace sek::math::detail