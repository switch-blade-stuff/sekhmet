/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2022-03-14
 */

#include "sysrandom.hpp"

#ifdef SEK_OS_LINUX

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
#define SEK_GETRANDOM
#endif
#endif

#ifdef SEK_GETRANDOM

#include <linux/random.h>
#include <sys/syscall.h>

ssize_t sek::math::sys_random(void *dst, std::size_t len) noexcept { return syscall(SYS_getrandom, dst, len, 0); }

#elif defined(SEK_OS_WIN) && defined(_MSC_VER)

// clang-format off
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <bcrypt.h>
// clang-format on

#pragma comment(lib, "bcrypt.lib")

ssize_t sek::math::sys_random(void *dst, std::size_t len) noexcept
{
	BCRYPT_ALG_HANDLE rng_alg;
	if (BCryptOpenAlgorithmProvider(&rng_alg, BCRYPT_RNG_ALGORITHM, nullptr, 0) != STATUS_SUCCESS) [[unlikely]]
		return -1;

	auto result = static_cast<ssize_t>(len);
	if (BCryptGenRandom(rng_alg, static_cast<PUCHAR>(dst), len, 0) != STATUS_SUCCESS) [[unlikely]]
		result = -1;
	if (BCryptCloseAlgorithmProvider(rng_alg, 0) != STATUS_SUCCESS) [[unlikely]]
		result = -1;
	return result;
}

#elif defined(__OpenBSD__)
#include <unistd.h>

ssize_t sek::math::sys_random(void *dst, std::size_t len) noexcept { return getentropy(dst, len); }

#elif defined(SEK_OS_UNIX)

#include <cstdio>

ssize_t sek::math::sys_random(void *dest, std::size_t len) noexcept
{
	if (auto urandom = fopen("/dev/urandom", "rb"); urandom) [[likely]]
	{
		if (fread(dst, len, 1, urandom) == len) [[likely]]
			return static_cast<ssize_t>(len);
	}
	return -1;
}

#else

ssize_t sek::math::sys_random(void *, std::size_t) noexcept { return -1; }

#endif
