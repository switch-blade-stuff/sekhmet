//
// Created by switchblade on 2022-03-14.
//

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

std::size_t sek::math::detail::sys_random(void *dst, std::size_t len) noexcept
{
	return syscall(SYS_getrandom, dst, len, 0);
}

#elif defined(SEK_OS_WIN)

#include <wincrypt.h>

static bool acquire_context(HCRYPTPROV *ctx) noexcept
{
	if (!CryptAcquireContext(ctx, nullptr, nullptr, PROV_RSA_FULL, 0)) [[unlikely]]
		return CryptAcquireContext(ctx, nullptr, nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET);
	return true;
}
std::size_t sek::math::detail::sys_random(void *dst, std::size_t len) noexcept
{
	HCRYPTPROV ctx;
	if (!acquire_context(&ctx)) [[unlikely]]
		return -1;
	if (!CryptGenRandom(ctx, len, static_cast<BYTE *>(dst))) [[unlikely]]
		return -1;
	if (!CryptReleaseContext(ctx, 0)) [[unlikely]]
		return -1;
}

#elif defined(__OpenBSD__)
#include <unistd.h>

std::size_t sek::math::detail::sys_random(void *dst, std::size_t len) noexcept
{
	return getentropy(dst, len);
}

#elif defined(SEK_OS_UNIX)

#include <cstdio>

std::size_t sek::math::detail::sys_random(void *dest, std::size_t len) noexcept
{
	if (auto urandom = fopen("/dev/urandom", "rb"); urandom) [[likely]]
	{
		if (fread(dst, len, 1, urandom) == len) [[likely]]
			return len;
	}

	return -1;
}

#else

#include <cstdlib>

std::size_t sek::math::detail::sys_random(void *dest, std::size_t len) noexcept
{
	auto bytes = static_cast<std::uint8_t *>(dest);
	while (len-- > 0) bytes[len] = static_cast<std::uint8_t>(rand());
	return len;
}

#endif
