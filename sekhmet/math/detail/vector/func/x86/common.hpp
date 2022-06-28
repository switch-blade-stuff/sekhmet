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

namespace sek::math::detail
{
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

	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<float, 3, P>
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
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<float, 4, P>
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
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<float, 3, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(float x, float y, float z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const float (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(3, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		float values[3];
		__m128 simd;
	};
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<float, 4, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(float x, float y, float z, float w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const float (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(4, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		float values[4];
		__m128 simd;
	};

	template<std::size_t N, storage_policy P>
	inline __m128 x86_pack_ps(const vector_data<float, N, P> &v) noexcept
	{
		if constexpr (N == 2)
			return _mm_set_ps(0, v[1], 0, v[0]);
		else if constexpr (N == 3)
			return _mm_set_ps(0, v[2], v[1], v[0]);
		else
			return _mm_set_ps(v[3], v[2], v[1], v[0]);
	}
	template<std::size_t N, storage_policy P>
	inline void x86_unpack_ps(vector_data<float, N, P> &out, __m128 v) noexcept
	{
		if constexpr (N == 2)
		{
			out[1] = _mm_cvtss_f32(_mm_unpackhi_ps(v, v));
			out[0] = _mm_cvtss_f32(v);
		}
		else if constexpr (N == 3)
		{
			const auto h = _mm_unpackhi_ps(v, v);
			const auto l = _mm_unpacklo_ps(v, v);
			out[2] = _mm_cvtss_f32(h);
			out[1] = _mm_cvtss_f32(_mm_unpackhi_ps(l, l));
			out[0] = _mm_cvtss_f32(l);
		}
		else
		{
			const auto h = _mm_unpackhi_ps(v, v);
			const auto l = _mm_unpacklo_ps(v, v);
			out[3] = _mm_cvtss_f32(_mm_unpackhi_ps(h, h));
			out[2] = _mm_cvtss_f32(h);
			out[1] = _mm_cvtss_f32(_mm_unpackhi_ps(l, l));
			out[0] = _mm_cvtss_f32(l);
		}
	}

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

	template<integral_of_size<4> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<T, 3, P>
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
	template<integral_of_size<4> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<T, 4, P>
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
	template<integral_of_size<4> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<T, 3, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(3, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[3];
		__m128i simd;
	};
	template<integral_of_size<4> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<T, 4, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z, T w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(4, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[4];
		__m128i simd;
	};

	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline __m128i x86_pack_ps(const vector_data<T, N, P> &v) noexcept
	{
		// clang-format off
		if constexpr (N == 2)
			return _mm_set_epi32(0, static_cast<std::int32_t>(v[1]),
								 0, static_cast<std::int32_t>(v[0]));
		else if constexpr (N == 3)
			return _mm_set_epi32(0, static_cast<std::int32_t>(v[2]),
								 static_cast<std::int32_t>(v[1]),
								 static_cast<std::int32_t>(v[0]));
		else
			return _mm_set_epi32(static_cast<std::int32_t>(v[3]),
								 static_cast<std::int32_t>(v[2]),
								 static_cast<std::int32_t>(v[1]),
								 static_cast<std::int32_t>(v[0]));
		// clang-format on
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void x86_unpack_ps(vector_data<T, N, P> &out, __m128i v) noexcept
	{
		if constexpr (N == 2)
		{
			out[1] = static_cast<T>(_mm_cvtsi128_si32(_mm_unpackhi_epi32(v, v)));
			out[0] = static_cast<T>(_mm_cvtsi128_si32(v));
		}
		else if constexpr (N == 3)
		{
			const auto h = _mm_unpackhi_epi32(v, v);
			const auto l = _mm_unpacklo_epi32(v, v);
			out[2] = static_cast<T>(_mm_cvtsi128_si32(h));
			out[1] = static_cast<T>(_mm_cvtsi128_si32(_mm_unpackhi_epi32(l, l)));
			out[0] = static_cast<T>(_mm_cvtsi128_si32(l));
		}
		else
		{
			const auto h = _mm_unpackhi_epi32(v, v);
			const auto l = _mm_unpacklo_epi32(v, v);
			out[3] = static_cast<T>(_mm_cvtsi128_si32(_mm_unpackhi_epi32(h, h)));
			out[2] = static_cast<T>(_mm_cvtsi128_si32(h));
			out[1] = static_cast<T>(_mm_cvtsi128_si32(_mm_unpackhi_epi32(l, l)));
			out[0] = static_cast<T>(_mm_cvtsi128_si32(l));
		}
	}

	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<double, 2, P>
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
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<double, 2, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y) noexcept : values{x, y} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(2, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[2];
		__m128d simd;
	};

	template<std::size_t N, storage_policy P>
	inline __m128d x86_pack_pd(const vector_data<double, 2, P> &v) noexcept
	{
		return _mm_set_pd(v[1], v[0]);
	}
	template<std::size_t N, storage_policy P>
	inline void x86_unpack_pd(vector_data<double, 2, P> &out, __m128d v) noexcept
	{
		out[1] = _mm_cvtsd_f64(_mm_unpackhi_pd(v, v));
		out[0] = _mm_cvtsd_f64(v);
	}

	template<integral_of_size<8> T, storage_policy P>
	union mask_data<T, 2, P>
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
	template<integral_of_size<8> T, storage_policy P>
	union vector_data<T, 2, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y) noexcept : values{x, y} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(2, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[2];
		__m128i simd;
	};

	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline __m128i x86_pack_pd(const vector_data<T, 2, P> &v) noexcept
	{
		return _mm_set_epi64x(static_cast<std::int64_t>(v[1]), static_cast<std::int64_t>(v[0]));
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void x86_unpack_pd(vector_data<T, 2, P> &out, __m128i v) noexcept
	{
		out[1] = static_cast<T>(_mm_cvtsi128_si64x(_mm_unpackhi_epi64(v, v)));
		out[0] = static_cast<T>(_mm_cvtsi128_si64x(v));
	}

#ifndef SEK_USE_AVX
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<double, 3, P>
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
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<double, 4, P>
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
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<double, 3, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y, double z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(3, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[3];
		__m128d simd[2];
	};
	template<storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<double, 4, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y, double z, double w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(4, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[4];
		__m128d simd[2];
	};

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<T, 3, P>
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
	template<integral_of_size<8> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union mask_data<T, 4, P>
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
	template<integral_of_size<8> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<T, 3, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(3, M); ++i) values[i] = data[i];
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[3];
		__m128i simd[2];
	};
	template<integral_of_size<8> T, storage_policy P>
		requires(P != storage_policy::SIZE)
	union vector_data<T, 4, P>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z, T w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(4, M); ++i) values[i] = data[i];
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