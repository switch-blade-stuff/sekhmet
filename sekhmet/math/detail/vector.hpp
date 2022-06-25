/*
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include "mask.hpp"
#include "shuffle.hpp"
#include "storage.hpp"
#include "util.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/vector_double.hpp"
#include "x86/vector_float.hpp"
#include "x86/vector_int32.hpp"
#include "x86/vector_int64.hpp"
#else
#warning "SMID vector operations are not implemented for this CPU"
#define SEK_NO_SIMD
#endif
#endif

#include "generic/vector.hpp"

#define SEK_DETAIL_VECTOR_COMMON(T, N, P)                                                                                         \
private:                                                                                                                          \
	using data_t = detail::vector_data<T, N, P>;                                                                                  \
	using mask_t = vec_mask<basic_vec>;                                                                                           \
	data_t m_data = {};                                                                                                           \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr sek::hash_t hash(const basic_vec<U, M, Sp> &) noexcept;                                                      \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr void swap(basic_vec<U, M, Sp> &, basic_vec<U, M, Sp> &) noexcept;                                            \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator+=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator-=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator%=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                 \
                                                                                                                                  \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> &operator&=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> operator&(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> &operator|=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> operator|(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> operator^(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;            \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> &operator^=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> operator~(const basic_vec<U, M, Sp> &) noexcept;                                         \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &) noexcept                                          \
		requires std::is_signed_v<U>                                                                                              \
	;                                                                                                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &) noexcept                                          \
		requires std::is_signed_v<U>                                                                                              \
	;                                                                                                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> abs(const basic_vec<U, M, Sp> &) noexcept                                                \
		requires std::is_signed_v<U>                                                                                              \
	;                                                                                                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> max(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> min(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                  \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> lerp(                                                                                    \
		const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                          \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> exp(const basic_vec<U, M, Sp> &) noexcept;                                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> exp2(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> expm1(const basic_vec<U, M, Sp> &) noexcept;                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> log(const basic_vec<U, M, Sp> &) noexcept;                                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> log10(const basic_vec<U, M, Sp> &) noexcept;                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> log2(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> log1p(const basic_vec<U, M, Sp> &) noexcept;                                             \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> sqrt(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> rsqrt(const basic_vec<U, M, Sp> &) noexcept;                                             \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> rad(const basic_vec<U, M, Sp> &) noexcept;                                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> deg(const basic_vec<U, M, Sp> &) noexcept;                                               \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> sin(const basic_vec<U, M, Sp> &) noexcept;                                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> cos(const basic_vec<U, M, Sp> &) noexcept;                                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> tan(const basic_vec<U, M, Sp> &) noexcept;                                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> asin(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> acos(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> atan(const basic_vec<U, M, Sp> &) noexcept;                                              \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> sinh(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> cosh(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> tanh(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> asinh(const basic_vec<U, M, Sp> &) noexcept;                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> acosh(const basic_vec<U, M, Sp> &) noexcept;                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> atanh(const basic_vec<U, M, Sp> &) noexcept;                                             \
                                                                                                                                  \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> round(const basic_vec<U, M, Sp> &) noexcept;                                             \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> floor(const basic_vec<U, M, Sp> &) noexcept;                                             \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> ceil(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> trunc(const basic_vec<U, M, Sp> &) noexcept;                                             \
                                                                                                                                  \
	template<typename U, storage_policy Sp>                                                                                       \
	friend constexpr basic_vec<U, 3, Sp> cross(const basic_vec<U, 3, Sp> &, const basic_vec<U, 3, Sp> &) noexcept                 \
		requires std::is_signed_v<U>                                                                                              \
	;                                                                                                                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr U dot(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> norm(const basic_vec<U, M, Sp> &) noexcept;                                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr U magn(const basic_vec<U, M, Sp> &) noexcept;                                                                \
                                                                                                                                  \
	template<std::size_t... I, typename U, std::size_t M, storage_policy Sp>                                                      \
	friend constexpr basic_vec<U, sizeof...(I), Sp> shuffle(const basic_vec<U, M, Sp> &) noexcept;                                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> interleave(                                                                              \
		const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator<(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator<=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator>(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator>=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
                                                                                                                                  \
public:                                                                                                                           \
	typedef T value_type;                                                                                                         \
	typedef mask_t mask_type;                                                                                                     \
                                                                                                                                  \
	constexpr static auto extent = N;                                                                                             \
	constexpr static auto policy = P;                                                                                             \
                                                                                                                                  \
	constexpr basic_vec() noexcept = default;                                                                                     \
                                                                                                                                  \
	template<std::convertible_to<T> U, std::size_t M, storage_policy OtherPolicy>                                                 \
	constexpr explicit basic_vec(const basic_vec<U, M, OtherPolicy> &other) noexcept                                              \
		requires(!std::same_as<T, U> || M != N || OtherPolicy != P)                                                               \
	{                                                                                                                             \
		for (std::size_t i = 0; i < min(M, N); ++i) m_data[i] = other.m_data[i];                                                  \
	}                                                                                                                             \
                                                                                                                                  \
	template<std::size_t M>                                                                                                       \
	constexpr explicit basic_vec(const value_type(&vals)[N]) noexcept : m_data(vals)                                              \
	{                                                                                                                             \
	}                                                                                                                             \
                                                                                                                                  \
	/** Returns the N-th element of the vector. */                                                                                \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) noexcept                                                     \
	{                                                                                                                             \
		return m_data[i];                                                                                                         \
	}                                                                                                                             \
	/** @copydoc operator[] */                                                                                                    \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) const noexcept                                               \
	{                                                                                                                             \
		return m_data[i];                                                                                                         \
	}                                                                                                                             \
	constexpr void swap(basic_vec &other) noexcept                                                                                \
	{                                                                                                                             \
		m_data.swap(other.m_data);                                                                                                \
	}

#define SEK_DETAIL_V_TYPE(Extent) basic_vec<T, Extent, Policy>
#define SEK_DETAIL_V_SHUFFLE(...) (shuffle<__VA_ARGS__>(*this))
#define SEK_DETAIL_V_SHUFFLE_2(x, y) SEK_DETAIL_SHUFFLE_2(SEK_DETAIL_V_SHUFFLE, SEK_DETAIL_V_TYPE, x, y)
#define SEK_DETAIL_V_SHUFFLE_3(x, y, z) SEK_DETAIL_SHUFFLE_3(SEK_DETAIL_V_SHUFFLE, SEK_DETAIL_V_TYPE, x, y, z)
#define SEK_DETAIL_V_SHUFFLE_4(x, y, z, w) SEK_DETAIL_SHUFFLE_4(SEK_DETAIL_V_SHUFFLE, SEK_DETAIL_V_TYPE, x, y, z, w)
#define SEK_VECTOR_GENERATE_SHUFFLE(x, ...)                                                                            \
	SEK_GET_MACRO_3(__VA_ARGS__, SEK_DETAIL_V_SHUFFLE_4, SEK_DETAIL_V_SHUFFLE_3, SEK_DETAIL_V_SHUFFLE_2)               \
	(x, __VA_ARGS__)

namespace sek::math
{
	/** @brief Structure representing a mathematical vector.
	 * @tparam T Type of values stored in the vector.
	 * @tparam N Amount of values the vector holds.
	 * @tparam Policy Policy used for storage & optimization. */
	template<typename T, std::size_t N, storage_policy Policy = storage_policy::OPTIMAL>
	class basic_vec;

	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 2, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_DETAIL_VECTOR_COMMON(T, 2, Policy)

	public:
		constexpr basic_vec(T x, T y) noexcept : m_data(x, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y)
	};
	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 3, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_DETAIL_VECTOR_COMMON(T, 3, Policy)

	public:
		constexpr basic_vec(T x, T y, T z) noexcept : m_data(x, y, z) {}
		constexpr basic_vec(T x, T y) noexcept : basic_vec(x, y, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z)

		[[nodiscard]] constexpr decltype(auto) r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) b() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b)
	};
	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 4, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_DETAIL_VECTOR_COMMON(T, 4, Policy)

	public:
		constexpr basic_vec(T x, T y, T z, T w) noexcept : m_data(x, y, z, w) {}
		constexpr basic_vec(T x, T y, T z) noexcept : basic_vec(x, y, z, z) {}
		constexpr basic_vec(T x, T y) noexcept : basic_vec(x, y, y, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x, x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) w() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr decltype(auto) w() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z, w)

		[[nodiscard]] constexpr decltype(auto) r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) b() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) a() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr decltype(auto) a() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b, a)

		[[nodiscard]] constexpr decltype(auto) s() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) s() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) t() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) t() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) p() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) p() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) q() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr decltype(auto) q() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(s, t, p, q)
	};

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr sek::hash_t hash(const basic_vec<U, M, Sp> &v) noexcept
	{
		hash_t result = 0;
		for (std::size_t i = 0; i < M; ++i) hash_combine(result, v[i]);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr void swap(basic_vec<U, M, Sp> &a, basic_vec<U, M, Sp> &b) noexcept
	{
		a.swap(b);
	}

	/** Returns a vector which is the result of addition of two vectors. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_add(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Adds a vector to a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator+=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_add(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of subtraction of two vectors. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Subtracts a vector from a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator-=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(l.m_data, l.m_data, r.m_data);
		return l;
	}

	/** Returns a copy of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &v) noexcept
		requires std::is_signed_v<U>
	{
		return v;
	}
	/** Returns a negated copy of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &v) noexcept
		requires std::is_signed_v<U>
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_neg(result.m_data, v.m_data);
		else
			detail::vector_neg(result.m_data, v.m_data);
		return result;
	}

	/** Returns a copy of a vector multiplied by another vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_mul(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Multiplies vector by another vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_mul(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a copy of a vector divided by another vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_div(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Divides vector by another vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_div(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a copy of a vector multiplied by a scalar. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l * basic_vec<U, M, Sp>{r};
	}
	/** @copydoc operator* */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(U l, const basic_vec<U, M, Sp> &r) noexcept
	{
		return l * basic_vec<U, M, Sp>{r};
	}
	/** Multiplies vector by a scalar. */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l *= basic_vec<U, M, Sp>{r};
	}
	/** Returns a copy of a vector divided by a scalar. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l / basic_vec<U, M, Sp>{r};
	}
	/** Returns a vector produced by dividing a scalar by a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(U l, const basic_vec<U, M, Sp> &r) noexcept
	{
		return basic_vec<U, M, Sp>{l} / r;
	}
	/** Divides vector by a scalar. */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l /= basic_vec<U, M, Sp>{r};
	}

	/** Calculates modulus of two vectors. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mod(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_mod(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** @copydoc operator% */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mod(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_mod(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Calculates modulus of vector and a scalar. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l % basic_vec<U, M, Sp>{r};
	}
	/** @copydoc operator% */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l %= basic_vec<U, M, Sp>{r};
	}
	/** Calculates floating-point modulus of two vectors. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_fmod(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_fmod(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Calculates floating-point modulus of vector and a scalar. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return fmod(l, basic_vec<U, M, Sp>{r});
	}

	/** Calculates absolute value of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> abs(const basic_vec<U, M, Sp> &v) noexcept
		requires std::is_signed_v<U>
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_abs(result.m_data, v.m_data);
		else
			detail::vector_abs(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of maximum elements of a and b. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> max(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_max(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_max(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Returns a vector consisting of minimum elements of a and b. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> min(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_min(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_min(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Clamps elements of a vector between a minimum and a maximum. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		clamp(const basic_vec<U, M, Sp> &value, const basic_vec<U, M, Sp> &min_val, const basic_vec<U, M, Sp> &max_val) noexcept
	{
		return max(min_val, min(max_val, value));
	}

	// clang-format off
	/** Calculates linear interpolation or extrapolation between two vectors. Equivalent to `l + t * (r - l)`. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, const basic_vec<U, M, Sp> &t) noexcept
	{
		return l + t * (r - l);
	}
	/** @copydoc lerp */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, U t) noexcept
	{
		return lerp(l, r, basic_vec<U, M, Sp>{t});
	}
	// clang-format on

	/** Returns a vector of `e` raised to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_exp(result.m_data, v.m_data);
		else
			detail::vector_exp(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector of `2` raised to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp2(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_exp2(result.m_data, v.m_data);
		else
			detail::vector_exp2(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector of `e` raised to the given power, minus one. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> expm1(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_expm1(result.m_data, v.m_data);
		else
			detail::vector_expm1(result.m_data, v.m_data);
		return result;
	}
	/** Calculates natural logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log(result.m_data, v.m_data);
		else
			detail::vector_log(result.m_data, v.m_data);
		return result;
	}
	/** Calculates common logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log10(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log10(result.m_data, v.m_data);
		else
			detail::vector_log10(result.m_data, v.m_data);
		return result;
	}
	/** Calculates base-2 logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log2(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log2(result.m_data, v.m_data);
		else
			detail::vector_log2(result.m_data, v.m_data);
		return result;
	}
	/** Calculates natural logarithms of 1 plus a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log1p(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log1p(result.m_data, v.m_data);
		else
			detail::vector_log1p(result.m_data, v.m_data);
		return result;
	}

	/** Raises elements of a vector to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_pow(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_pow(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** @copydoc pow */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return pow(l, basic_vec<U, M, Sp>{r});
	}
	/** Calculates square root of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sqrt(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sqrt(result.m_data, v.m_data);
		else
			detail::vector_sqrt(result.m_data, v.m_data);
		return result;
	}
	/** Calculates cubic root of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cbrt(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cbrt(result.m_data, v.m_data);
		else
			detail::vector_cbrt(result.m_data, v.m_data);
		return result;
	}
	/** Calculates reciprocal square root of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> rsqrt(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_rsqrt(result.m_data, v.m_data);
		else
			detail::vector_rsqrt(result.m_data, v.m_data);
		return result;
	}

	/** Calculates a sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sin(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_sin(result.m_data, v.m_data);
		else
			detail::vector_sin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cos(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cos(result.m_data, v.m_data);
		else
			detail::vector_cos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tan(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_tan(result.m_data, v.m_data);
		else
			detail::vector_tan(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asin(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_asin(result.m_data, v.m_data);
		else
			detail::vector_asin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acos(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acos(result.m_data, v.m_data);
		else
			detail::vector_acos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atan(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_atan(result.m_data, v.m_data);
		else
			detail::vector_atan(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sinh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_sinh(result.m_data, v.m_data);
		else
			detail::vector_sinh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cosh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cosh(result.m_data, v.m_data);
		else
			detail::vector_cosh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tanh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_tanh(result.m_data, v.m_data);
		else
			detail::vector_tanh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asinh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_asinh(result.m_data, v.m_data);
		else
			detail::vector_asinh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acosh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acosh(result.m_data, v.m_data);
		else
			detail::vector_acosh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atanh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_atanh(result.m_data, v.m_data);
		else
			detail::vector_atanh(result.m_data, v.m_data);
		return result;
	}

	/** Returns a vector consisting of rounded values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> round(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_round(result.m_data, v.m_data);
		else
			detail::vector_round(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of rounded-down values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> floor(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_floor(result.m_data, v.m_data);
		else
			detail::vector_floor(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of rounded-up values of `v`.
	 * @example round({.1, .2, 2.3}) -> {1, 1, 3} */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> ceil(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_ceil(result.m_data, v.m_data);
		else
			detail::vector_ceil(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of truncated values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> trunc(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_trunc(result.m_data, v.m_data);
		else
			detail::vector_trunc(result.m_data, v.m_data);
		return result;
	}

	/** Preforms a bitwise AND on two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator&=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_and(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise AND of two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator&(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_and(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise OR on two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator|=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_or(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise OR of two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator|(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_or(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Returns a vector which is the result of bitwise XOR of two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator^(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise XOR on two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator^=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a bitwise inverted copy of a vector. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator~(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_inv(result.m_data, v.m_data);
		else
			detail::vector_inv(result.m_data, v.m_data);
		return result;
	}

	/** Calculates dot product of two vectors. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr U dot(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::generic::vector_dot(l.m_data, r.m_data);
		else
			return detail::vector_dot(l.m_data, r.m_data);
	}
	/** Calculates cross product of two vectors. */
	template<typename T, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, 3, Sp> cross(const basic_vec<T, 3, Sp> &l, const basic_vec<T, 3, Sp> &r) noexcept
		requires std::is_signed_v<T>
	{
		basic_vec<T, 3> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cross(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_cross(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Returns a length of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr U magn(const basic_vec<U, M, Sp> &v) noexcept
	{
		/* Magnitude of a vector A=XYZ is sqrt(X*X + Y*Y + Z*Z) = sqrt(dot(A, A)). */
		return std::sqrt(dot(v, v));
	}
	/** Returns a dist between two vectors. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr U dist(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		return magn(l - r);
	}
	/** Returns a normalized copy of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> norm(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_norm(result.m_data, v.m_data);
		else
			detail::vector_norm(result.m_data, v.m_data);
		return result;
	}

	/** Converts a degree angle vector to radian angle vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> rad(const basic_vec<U, M, Sp> &v) noexcept
	{
		return v * basic_vec<U, M, Sp>{std::numbers::pi_v<U> / static_cast<U>(180.0)};
	}
	/** Converts a radian angle vector to degree angle vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> deg(const basic_vec<U, M, Sp> &v) noexcept
	{
		return v * basic_vec<U, M, Sp>{static_cast<U>(180.0) / std::numbers::pi_v<U>};
	}

	/** Gets the Ith element of the vector. */
	template<std::size_t I, typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr decltype(auto) get(basic_vec<U, M, Sp> &v) noexcept
	{
		return v[I];
	}
	/** @copydoc get */
	template<std::size_t I, typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr decltype(auto) get(const basic_vec<U, M, Sp> &v) noexcept
	{
		return v[I];
	}

	namespace detail
	{
		template<std::size_t... Is, typename U, std::size_t M, storage_policy Sp, typename F>
		constexpr void vectorize_impl(std::index_sequence<Is...>, const basic_vec<U, M, Sp> &v, F &&f)
		{
			(f(get<Is>(v)), ...);
		}
		template<std::size_t... Is, typename U, std::size_t M, storage_policy Sp, typename F>
		constexpr void vectorize_impl(std::index_sequence<Is...>, basic_vec<U, M, Sp> &v, F &&f)
		{
			(f(get<Is>(v)), ...);
		}
	}	 // namespace detail

	/** Applies a functor to every element of the vector. */
	template<typename U, std::size_t M, storage_policy Sp, typename F>
	constexpr void vectorize(const basic_vec<U, M, Sp> &v, F &&f)
	{
		detail::vectorize_impl(std::make_index_sequence<M>{}, v, std::forward<F>(f));
	}
	/** @copydoc vectorize */
	template<typename U, std::size_t M, storage_policy Sp, typename F>
	constexpr void vectorize(basic_vec<U, M, Sp> &v, F &&f)
	{
		detail::vectorize_impl(std::make_index_sequence<M>{}, v, std::forward<F>(f));
	}

	/** Shuffles elements of a vector according to the provided indices.
	 * @tparam I Indices of elements of the source vector in the order they should be shuffled to the destination vector.
	 * @return Result vector who's elements are specified by `I`.
	 * @example shuffle<2, 1, 0>({3, 4, 5}) -> {5, 4, 3} */
	template<std::size_t... I, typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, sizeof...(I), Sp> shuffle(const basic_vec<U, M, Sp> &v) noexcept
	{
		using Idx = std::index_sequence<I...>;
		if constexpr (std::is_same_v<Idx, std::make_index_sequence<M>>)
			return v;
		else
		{
			basic_vec<U, sizeof...(I)> result;
			if (std::is_constant_evaluated())
				detail::generic::vector_shuffle(result.m_data, v.m_data, Idx{});
			else
				detail::vector_shuffle(result.m_data, v.m_data, Idx{});
			return result;
		}
	}

	/** Interleaves elements of two vectors according to the provided mask.
	 * @param l Left-hand vector.
	 * @param r Right-hand vector.
	 * @param mask Mask used to select vector elements.
	 * `true` will select the left-hand element, `false` will select the right-hand element.
	 * @return Result of the interleave operation. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> interleave(const basic_vec<U, M, Sp> &l,
														   const basic_vec<U, M, Sp> &r,
														   const vec_mask<basic_vec<U, M, Sp>> &mask) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_interleave(result.m_data, l.m_data, r.m_data, mask.m_data);
		else
			detail::vector_interleave(result.m_data, l.m_data, r.m_data, mask.m_data);
		return result;
	}

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_eq(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_eq(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ne(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ne(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator<(const basic_vec<U, M, Sp> &l,
																	const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_lt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_lt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator<=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_le(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_le(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator>(const basic_vec<U, M, Sp> &l,
																	const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_gt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_gt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator>=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ge(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ge(result.m_data, l.m_data, r.m_data);
		return result;
	}

	/** Checks if elements of vector a equals vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_eq(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return abs(a - b) <= epsilon;
	}
	/** @copydoc fcmp_eq */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_eq(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fcmp_eq(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a does not equal vector vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ne(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return abs(a - b) > epsilon;
	}
	/** @copydoc fcmp_ne */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_ne(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fcmp_ne(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is less than or equal to vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_le(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a <= b || fcmp_eq(a, b, epsilon);
	}
	/** @copydoc fcmp_le */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_le(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fcmp_le(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is greater than or equal to vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ge(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a >= b || fcmp_eq(a, b, epsilon);
	}
	/** @copydoc fcmp_ge */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_ge(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fcmp_ge(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is less than vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_lt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a < b && fcmp_ne(a, b, epsilon);
	}
	/** @copydoc fcmp_lt */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_lt(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fcmp_lt(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is less than vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_gt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a > b && fcmp_ne(a, b, epsilon);
	}
	/** @copydoc fcmp_gt */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_gt(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fcmp_gt(a, b, basic_vec<U, M, Sp>{epsilon});
	}

	/** Returns a vector consisting of minimum elements of a and b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmin(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return interleave(a, b, fcmp_le(a, b, epsilon));
	}
	/** @copydoc fmin */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmin(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fmin(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Returns a vector consisting of masimum elements of a and b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmax(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return interleave(a, b, fcmp_ge(a, b, epsilon));
	}
	/** @copydoc fmax */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmax(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fmax(a, b, basic_vec<U, M, Sp>{epsilon});
	}

	/** Clamps elements of a vector between a minimum and a maximum using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fclamp(const basic_vec<U, M, Sp> &value,
													   const basic_vec<U, M, Sp> &min_val,
													   const basic_vec<U, M, Sp> &max_val,
													   const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return fmax(min_val, fmin(max_val, value, epsilon), epsilon);
	}
	/** @copydoc fclamp */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fclamp(const basic_vec<U, M, Sp> &value,
													   const basic_vec<U, M, Sp> &min_val,
													   const basic_vec<U, M, Sp> &max_val,
													   U epsilon = std::numeric_limits<U>::epsilon()) noexcept
	{
		return fclamp(value, min_val, max_val, basic_vec<U, M, Sp>{epsilon});
	}
}	 // namespace sek::math

template<typename U, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_size<sek::math::basic_vec<U, M, Sp>> : std::integral_constant<std::size_t, M>
{
};
template<std::size_t I, typename U, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_element<I, sek::math::basic_vec<U, M, Sp>>
{
	using type = U;
};
