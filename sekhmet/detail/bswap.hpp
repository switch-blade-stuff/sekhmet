//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include "arch.h"

#ifdef SEK_OS_LINUX

#include <byteswap.h>

#else
#ifdef _MSC_VER

#include <cstdlib>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif SEK_OS_APPLE

#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <machine/bswap.h>
#include <sys/types.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#endif

#ifndef bswap_16
#include <bit>
#include <cstdint>
constexpr static auto bswap_16(auto value) noexcept
{
	auto *data = std::bit_cast<std::byte *>(&value);
	std::swap(data[0], data[1]);
	return value;
}
#endif
#endif

#ifdef SEK_ARCH_LITTLE_ENDIAN
#define BSWAP_LE_16(x) static_cast<std::uint16_t>(x)
#define BSWAP_LE_32(x) static_cast<std::uint32_t>(x)
#define BSWAP_LE_64(x) static_cast<std::uint64_t>(x)
#define BSWAP_BE_16(x) bswap_16(static_cast<std::uint16_t>(x))
#define BSWAP_BE_32(x) bswap_32(static_cast<std::uint32_t>(x))
#define BSWAP_BE_64(x) bswap_64(static_cast<std::uint64_t>(x))
#else
#define BSWAP_LE_16(x) bswap_16(static_cast<std::uint16_t>(x))
#define BSWAP_LE_32(x) bswap_32(static_cast<std::uint32_t>(x))
#define BSWAP_LE_64(x) bswap_64(static_cast<std::uint64_t>(x))
#define BSWAP_BE_16(x) static_cast<std::uint16_t>(x)
#define BSWAP_BE_32(x) static_cast<std::uint32_t>(x)
#define BSWAP_BE_64(x) static_cast<std::uint64_t>(x)
#endif