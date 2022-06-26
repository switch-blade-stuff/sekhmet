/*
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include <bit>

#include "../../storage.hpp"

#ifdef SEK_ARCH_x86

#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <xmmintrin.h>

#ifdef SEK_NO_SIMD
#ifdef SEK_USE_SSE
#undef SEK_USE_SSE
#endif
#ifdef SEK_USE_AVX
#undef SEK_USE_AVX
#endif
#endif

#if defined(SEK_USE_SSE) && (!defined(__SSE__))
#undef SEK_USE_SSE
#endif
#if defined(SEK_USE_SSE2) && (!defined(__SSE2__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSE2
#endif
#if defined(SEK_USE_SSE3) && (!defined(__SSE3__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSE3
#endif
#if defined(SEK_USE_SSSE3) && (!defined(__SSSE3__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSSE3
#endif
#if defined(SEK_USE_SSE4) && !defined(SEK_USE_SSE)
#undef SEK_USE_SSE4
#endif
#if defined(SEK_USE_SSE4_1) && (!defined(__SSE4_1__) || !defined(SEK_USE_SSE4))
#undef SEK_USE_SSE4_1
#endif
#if defined(SEK_USE_SSE4_2) && (!defined(__SSE4_2__) || !defined(SEK_USE_SSE4))
#undef SEK_USE_SSE4_2
#endif

#if defined(SEK_USE_AVX) && !defined(__AVX__)
#undef SEK_USE_AVX
#endif
#if defined(SEK_USE_AVX2) && (!defined(__AVX2__) || !defined(SEK_USE_AVX))
#undef SEK_USE_AVX2
#endif

// clang-format off
#define SEK_DETAIL_IS_SIMD_1(a) (requires{ (a).simd; })
#define SEK_DETAIL_IS_SIMD_2(a, b) (requires{ (a).simd; } && requires{ (b).simd; })
#define SEK_DETAIL_IS_SIMD_3(a, b, c) (requires{ (a).simd; } && requires{ (b).simd; } && requires{ (c).simd; })
// clang-format on

#define SEK_DETAIL_IS_SIMD(...)                                                                                        \
	SEK_GET_MACRO_3(__VA_ARGS__, SEK_DETAIL_IS_SIMD_3, SEK_DETAIL_IS_SIMD_2, SEK_DETAIL_IS_SIMD_1)(__VA_ARGS__)

namespace sek::math::detail
{
	template<typename T, std::size_t N>
	using simd_vector = vector_data<T, N, storage_policy::OPTIMAL>;
	template<typename T, std::size_t N>
	using simd_mask = mask_data<T, N, storage_policy::OPTIMAL>;

	template<typename Data>
	concept simd_enabled = requires(Data data) { data.simd; };

	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle4_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_128_shuffle4_unwrap<J + 2>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle4_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_128_shuffle4_unwrap<0>(s);
	}

	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle2_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_128_shuffle2_unwrap<J + 1>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle2_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_128_shuffle2_unwrap<0>(s);
	}

#ifdef SEK_USE_SSE
	template<>
	struct mask_set<std::uint32_t>
	{
		template<typename U>
		constexpr void operator()(std::uint32_t &to, U &&from) const noexcept
		{
			to = from ? std::numeric_limits<std::uint32_t>::max() : 0;
		}
	};
	template<>
	struct mask_get<std::uint32_t>
	{
		constexpr bool operator()(auto &v) const noexcept { return v; }
	};

	template<>
	union mask_data<float, 3, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(3, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[3];
		__m128 simd;
	};
	template<>
	union mask_data<float, 4, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z, bool w) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
			operator[](3) = w;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(4, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[4];
		__m128 simd;
	};
	template<>
	union vector_data<float, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(float x, float y, float z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const float (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		float values[3];
		__m128 simd;
	};
	template<>
	union vector_data<float, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(float x, float y, float z, float w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const float (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		float values[4];
		__m128 simd;
	};

#ifdef SEK_USE_SSE2
	template<>
	struct mask_set<std::uint64_t>
	{
		template<typename U>
		constexpr void operator()(std::uint64_t &to, U &&from) const noexcept
		{
			to = from ? std::numeric_limits<std::uint64_t>::max() : 0;
		}
	};
	template<>
	struct mask_get<std::uint64_t>
	{
		constexpr bool operator()(auto &v) const noexcept { return v; }
	};

	template<integral_of_size<4> T>
	union mask_data<T, 3, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(3, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[3];
		__m128i simd;
	};
	template<integral_of_size<4> T>
	union mask_data<T, 4, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z, bool w) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
			operator[](3) = w;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(4, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[4];
		__m128i simd;
	};

	template<integral_of_size<4> T>
	union vector_data<T, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[3];
		__m128i simd;
	};
	template<integral_of_size<4> T>
	union vector_data<T, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z, T w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[4];
		__m128i simd;
	};

	template<integral_of_size<8> T>
	union mask_data<T, 2, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(2, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[2];
		__m128i simd;
	};
	template<integral_of_size<8> T>
	union vector_data<T, 2, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y) noexcept : values{x, y} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(2, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[2];
		__m128i simd;
	};

	template<>
	union mask_data<double, 2, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(2, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[2];
		__m128d simd;
	};
	template<>
	union vector_data<double, 2, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y) noexcept : values{x, y} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(2, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[2];
		__m128d simd;
	};

#ifndef SEK_USE_AVX
	template<>
	union mask_data<double, 3, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(3, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[3];
		__m128d simd[2];
	};
	template<>
	union mask_data<double, 4, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z, bool w) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
			operator[](3) = w;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(4, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[4];
		__m128d simd[2];
	};
	template<>
	union vector_data<double, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y, double z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[3];
		__m128d simd[2];
	};
	template<>
	union vector_data<double, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y, double z, double w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[4];
		__m128d simd[2];
	};

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T>
	union mask_data<T, 3, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(3, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[3];
		__m128i simd[2];
	};
	template<integral_of_size<8> T>
	union mask_data<T, 4, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z, bool w) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
			operator[](3) = w;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(4, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[4];
		__m128i simd[2];
	};
	template<integral_of_size<8> T>
	union vector_data<T, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[3];
		__m128i simd[2];
	};
	template<integral_of_size<8> T>
	union vector_data<T, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z, T w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[4];
		__m128i simd[2];
	};
#endif
#endif
#endif
#endif
}	 // namespace sek::math::detail

#endif