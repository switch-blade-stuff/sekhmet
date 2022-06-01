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
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include "sekhmet/detail/hash.hpp"
#include "util.hpp"
#include "vector_data.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/vector_impl.hpp"
#else
#warning "SMID vector operations are not implemented for this CPU"
#define SEK_NO_SIMD
#endif
#endif

#include "generic/vector_impl.hpp"

/* Use a macro to avoid duplicating common definitions for vector specializations. */
#define SEK_MATH_VECTOR_COMMON(T, N, P)                                                                                         \
private:                                                                                                                        \
	detail::vector_data_t<T, N, P> data = {};                                                                                   \
                                                                                                                                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr sek::hash_t hash(const basic_vector<U, M, Sp> &) noexcept;                                                 \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr void swap(basic_vector<U, M, Sp> &, basic_vector<U, M, Sp> &) noexcept;                                    \
                                                                                                                                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr auto operator<=>(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;                 \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr bool operator==(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> operator+(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> &operator+=(basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;     \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> operator-(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> &operator-=(basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;     \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> operator*(const basic_vector<U, M, Sp> &, U) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> &operator*=(basic_vector<U, M, Sp> &, U) noexcept;                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> operator/(const basic_vector<U, M, Sp> &, U) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> &operator/=(basic_vector<U, M, Sp> &, U) noexcept;                                  \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                 \
	friend constexpr basic_vector<U, M, Sp> &operator&=(basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;     \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                 \
	friend constexpr basic_vector<U, M, Sp> operator&(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept; \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                 \
	friend constexpr basic_vector<U, M, Sp> &operator|=(basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;     \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                 \
	friend constexpr basic_vector<U, M, Sp> operator|(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept; \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                 \
	friend constexpr basic_vector<U, M, Sp> operator^(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept; \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                 \
	friend constexpr basic_vector<U, M, Sp> &operator^=(basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;     \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                 \
	friend constexpr basic_vector<U, M, Sp> operator~(const basic_vector<U, M, Sp> &) noexcept;                                 \
                                                                                                                                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> operator+(const basic_vector<U, M, Sp> &) noexcept                                  \
		requires std::is_signed_v<U>                                                                                            \
	;                                                                                                                           \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> operator-(const basic_vector<U, M, Sp> &) noexcept                                  \
		requires std::is_signed_v<U>                                                                                            \
	;                                                                                                                           \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> abs(const basic_vector<U, M, Sp> &) noexcept                                        \
		requires std::is_signed_v<U>                                                                                            \
	;                                                                                                                           \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> max(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;       \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> min(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;       \
                                                                                                                                \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> sqrt(const basic_vector<U, M, Sp> &) noexcept;                                      \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> rsqrt(const basic_vector<U, M, Sp> &) noexcept;                                     \
                                                                                                                                \
	template<typename U, storage_policy Sp>                                                                                     \
	friend constexpr basic_vector<U, 3, Sp> cross(const basic_vector<U, 3, Sp> &, const basic_vector<U, 3, Sp> &) noexcept      \
		requires std::is_signed_v<U>                                                                                            \
	;                                                                                                                           \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr U dot(const basic_vector<U, M, Sp> &, const basic_vector<U, M, Sp> &) noexcept;                            \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr basic_vector<U, M, Sp> norm(const basic_vector<U, M, Sp> &) noexcept;                                      \
	template<typename U, std::size_t M, storage_policy Sp>                                                                      \
	friend constexpr U magn(const basic_vector<U, M, Sp> &) noexcept;                                                           \
                                                                                                                                \
	template<std::size_t... I, typename U, std::size_t M, storage_policy Sp>                                                    \
	friend constexpr basic_vector<U, sizeof...(I), Sp> shuffle(const basic_vector<U, M, Sp> &) noexcept;                        \
                                                                                                                                \
	template<std::size_t I, typename U, std::size_t M, storage_policy Sp>                                                       \
	friend constexpr U &get(basic_vector<U, M, Sp> &) noexcept;                                                                 \
	template<std::size_t I, typename U, std::size_t M, storage_policy Sp>                                                       \
	friend constexpr const U &get(const basic_vector<U, M, Sp> &) noexcept;                                                     \
                                                                                                                                \
public:                                                                                                                         \
	typedef T value_type;                                                                                                       \
                                                                                                                                \
	constexpr basic_vector() noexcept = default;                                                                                \
                                                                                                                                \
	template<std::size_t M, storage_policy OtherPolicy>                                                                         \
	constexpr explicit basic_vector(const basic_vector<T, M, OtherPolicy> &other) noexcept                                      \
		requires(M != N || OtherPolicy != P)                                                                                    \
	: data(other.data)                                                                                                          \
	{                                                                                                                           \
	}                                                                                                                           \
                                                                                                                                \
	constexpr explicit basic_vector(const value_type(&vals)[N]) noexcept : data(vals)                                           \
	{                                                                                                                           \
	}                                                                                                                           \
                                                                                                                                \
	[[nodiscard]] constexpr value_type &operator[](std::size_t i) noexcept                                                      \
	{                                                                                                                           \
		return data[i];                                                                                                         \
	}                                                                                                                           \
	[[nodiscard]] constexpr const value_type &operator[](std::size_t i) const noexcept                                          \
	{                                                                                                                           \
		return data[i];                                                                                                         \
	}                                                                                                                           \
	constexpr void swap(basic_vector &other) noexcept                                                                           \
	{                                                                                                                           \
		data.swap(other.data);                                                                                                  \
	}

namespace sek::math
{
	/** Generic vector.
	 * @tparam T Type of values stored in the vector.
	 * @tparam N Amount of values the vector holds.
	 * @tparam Policy Policy used for storage & optimization.
	 * @note Generic vector types are not guaranteed to be SIMD-optimized. */
	template<arithmetic T, std::size_t N, storage_policy Policy = storage_policy::OPTIMAL>
	union basic_vector
	{
		template<arithmetic U, std::size_t M, storage_policy P>
		friend union basic_vector;

		SEK_MATH_VECTOR_COMMON(T, N, Policy)

	private:
		template<typename... U>
		constexpr static bool compatible_type_seq = std::conjunction_v<std::is_convertible<U, T>...>;

	public:
		template<arithmetic... U>
		constexpr explicit basic_vector(U... data) noexcept
			requires compatible_type_seq<U...>
		: data({data...})
		{
		}
	};

	template<arithmetic T, storage_policy Policy>
	union basic_vector<T, 2, Policy>
	{
		template<arithmetic U, std::size_t M, storage_policy P>
		friend union basic_vector;

		SEK_MATH_VECTOR_COMMON(T, 2, Policy)

		constexpr basic_vector(T x, T y) noexcept : data({x, y}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}) {}

		[[nodiscard]] constexpr T &x() noexcept { return data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return data[1]; }
	};

	template<arithmetic T, storage_policy Policy>
	union basic_vector<T, 3, Policy>
	{
		template<arithmetic U, std::size_t M, storage_policy P>
		friend union basic_vector;

		SEK_MATH_VECTOR_COMMON(T, 3, Policy)

		constexpr basic_vector(T x, T y, T z) noexcept : data({x, y, z}) {}
		constexpr basic_vector(T x, T y) noexcept : basic_vector(x, y, T{}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}, T{}) {}

		[[nodiscard]] constexpr T &x() noexcept { return data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return data[1]; }
		[[nodiscard]] constexpr T &z() noexcept { return data[2]; }
		[[nodiscard]] constexpr const T &z() const noexcept { return data[2]; }

		[[nodiscard]] constexpr T &r() noexcept { return data[0]; }
		[[nodiscard]] constexpr const T &r() const noexcept { return data[0]; }
		[[nodiscard]] constexpr T &g() noexcept { return data[1]; }
		[[nodiscard]] constexpr const T &g() const noexcept { return data[1]; }
		[[nodiscard]] constexpr T &b() noexcept { return data[2]; }
		[[nodiscard]] constexpr const T &b() const noexcept { return data[2]; }
	};

	template<arithmetic T, storage_policy Policy>
	union basic_vector<T, 4, Policy>
	{
		template<arithmetic U, std::size_t M, storage_policy P>
		friend union basic_vector;

		SEK_MATH_VECTOR_COMMON(T, 4, Policy)

		constexpr basic_vector(T x, T y, T z, T w) noexcept : data({x, y, z, w}) {}
		constexpr basic_vector(T x, T y, T z) noexcept : basic_vector(x, y, z, T{}) {}
		constexpr basic_vector(T x, T y) noexcept : basic_vector(x, y, T{}, T{}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}, T{}, T{}) {}

		[[nodiscard]] constexpr T &x() noexcept { return data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return data[1]; }
		[[nodiscard]] constexpr T &z() noexcept { return data[2]; }
		[[nodiscard]] constexpr const T &z() const noexcept { return data[2]; }
		[[nodiscard]] constexpr T &w() noexcept { return data[3]; }
		[[nodiscard]] constexpr const T &w() const noexcept { return data[3]; }

		[[nodiscard]] constexpr T &r() noexcept { return data[0]; }
		[[nodiscard]] constexpr const T &r() const noexcept { return data[0]; }
		[[nodiscard]] constexpr T &g() noexcept { return data[1]; }
		[[nodiscard]] constexpr const T &g() const noexcept { return data[1]; }
		[[nodiscard]] constexpr T &b() noexcept { return data[2]; }
		[[nodiscard]] constexpr const T &b() const noexcept { return data[2]; }
		[[nodiscard]] constexpr T &a() noexcept { return data[3]; }
		[[nodiscard]] constexpr const T &a() const noexcept { return data[3]; }

		[[nodiscard]] constexpr T &s() noexcept { return data[0]; }
		[[nodiscard]] constexpr const T &s() const noexcept { return data[0]; }
		[[nodiscard]] constexpr T &t() noexcept { return data[1]; }
		[[nodiscard]] constexpr const T &t() const noexcept { return data[1]; }
		[[nodiscard]] constexpr T &p() noexcept { return data[2]; }
		[[nodiscard]] constexpr const T &p() const noexcept { return data[2]; }
		[[nodiscard]] constexpr T &q() noexcept { return data[3]; }
		[[nodiscard]] constexpr const T &q() const noexcept { return data[3]; }
	};

	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr sek::hash_t hash(const basic_vector<T, N, Sp> &v) noexcept
	{
		return v.data.hash();
	}
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr void swap(basic_vector<T, N, Sp> &a, basic_vector<T, N, Sp> &b) noexcept
	{
		a.swap(b);
	}

	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr auto operator<=>(const basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		return l.data <=> r.data;
	}
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr bool operator==(const basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		return l.data == r.data;
	}

	/** Returns a vector which is the result of addition of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator+(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_add(result.data, l.data, r.data);
		else
			detail::vector_add(result.data, l.data, r.data);
		return result;
	}
	/** Adds a vector to a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator+=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_add(l.data, l.data, r.data);
		else
			detail::vector_add(l.data, l.data, r.data);
		return l;
	}
	/** Returns a vector which is the result of subtraction of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator-(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(result.data, l.data, r.data);
		else
			detail::vector_sub(result.data, l.data, r.data);
		return result;
	}
	/** Subtracts a vector from a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator-=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(l.data, l.data, r.data);
		else
			detail::vector_sub(l.data, l.data, r.data);
		return l;
	}

	/** Returns a copy of a vector multiplied by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator*(const basic_vector<T, N, Sp> &l, T r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(result.data, l.data, r);
		else
			detail::vector_mul(result.data, l.data, r);
		return result;
	}
	/** @copydoc operator* */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator*(T l, const basic_vector<T, N, Sp> &r) noexcept
	{
		return r * l;
	}
	/** Multiplies vector by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator*=(basic_vector<T, N, Sp> &l, T r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.data, l.data, r);
		else
			detail::vector_mul(l.data, l.data, r);
		return l;
	}
	/** Returns a copy of a vector divided by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator/(const basic_vector<T, N, Sp> &l, T r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.data, l.data, r);
		else
			detail::vector_div(result.data, l.data, r);
		return result;
	}
	/** Returns a vector produced by dividing a scalar by a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator/(T l, const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.data, l, r.data);
		else
			detail::vector_div(result.data, l, r.data);
		return result;
	}
	/** Divides vector by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator/=(basic_vector<T, N, Sp> &l, T r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_div(l.data, l.data, r);
		else
			detail::vector_div(l.data, l.data, r);
		return l;
	}

	/** Preforms a bitwise AND on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator&=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_and(l.data, l.data, r.data);
		else
			detail::vector_and(l.data, l.data, r.data);
		return l;
	}
	/** Returns a vector which is the result of bitwise AND of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator&(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_and(result.data, l.data, r.data);
		else
			detail::vector_and(result.data, l.data, r.data);
		return result;
	}
	/** Preforms a bitwise OR on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator|=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_or(l.data, l.data, r.data);
		else
			detail::vector_or(l.data, l.data, r.data);
		return l;
	}
	/** Returns a vector which is the result of bitwise OR of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator|(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_or(result.data, l.data, r.data);
		else
			detail::vector_or(result.data, l.data, r.data);
		return result;
	}
	/** Returns a vector which is the result of bitwise XOR of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator^(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(result.data, l.data, r.data);
		else
			detail::vector_xor(result.data, l.data, r.data);
		return result;
	}
	/** Preforms a bitwise XOR on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator^=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(l.data, l.data, r.data);
		else
			detail::vector_xor(l.data, l.data, r.data);
		return l;
	}
	/** Returns a bitwise inverted copy of a vector. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator~(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_inv(result.data, v.data);
		else
			detail::vector_inv(result.data, v.data);
		return result;
	}

	/** Returns a copy of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator+(const basic_vector<T, N, Sp> &v) noexcept
		requires std::is_signed_v<T>
	{
		return v;
	}
	/** Returns a negated copy of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator-(const basic_vector<T, N, Sp> &v) noexcept
		requires std::is_signed_v<T>
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_neg(result.data, v.data);
		else
			detail::vector_neg(result.data, v.data);
		return result;
	}

	/** Calculates absolute value of a vector.
	 * @example abs({-1, 2, 0}) -> {1, 2, 0} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> abs(const basic_vector<T, N, Sp> &v) noexcept
		requires std::is_signed_v<T>
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_abs(result.data, v.data);
		else
			detail::vector_abs(result.data, v.data);
		return result;
	}
	/** Returns a vector consisting of maximum data of a and b.
	 * @example max({0, 1, 3}, {-1, 2, 2}) -> {0, 2, 3} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> max(const basic_vector<T, N, Sp> &a, const basic_vector<T, N, Sp> &b) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_max(result.data, a.data, b.data);
		else
			detail::vector_max(result.data, a.data, b.data);
		return result;
	}
	/** Returns a vector consisting of minimum data of a and b.
	 * @example min({0, 1, 3}, {-1, 2, 2}) -> {-1, 1, 2} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> min(const basic_vector<T, N, Sp> &a, const basic_vector<T, N, Sp> &b) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_min(result.data, a.data, b.data);
		else
			detail::vector_min(result.data, a.data, b.data);
		return result;
	}

	/** Calculates square root of a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> sqrt(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sqrt(result.data, v.data);
		else
			detail::vector_sqrt(result.data, v.data);
		return result;
	}
	/** Calculates reciprocal square root of a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> rsqrt(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_rsqrt(result.data, v.data);
		else
			detail::vector_rsqrt(result.data, v.data);
		return result;
	}

	/** Calculates dot product of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T dot(const basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::generic::vector_dot(l.data, r.data);
		else
			return detail::vector_dot(l.data, r.data);
	}
	/** Calculates cross product of two vectors. */
	template<typename T, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, 3, Sp> cross(const basic_vector<T, 3, Sp> &l, const basic_vector<T, 3, Sp> &r) noexcept
		requires std::is_signed_v<T>
	{
		basic_vector<T, 3> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cross(result.data, l.data, r.data);
		else
			detail::vector_cross(result.data, l.data, r.data);
		return result;
	}
	/** Returns a length of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T magn(const basic_vector<T, N, Sp> &v) noexcept
	{
		/* Magnitude of a vector A=XYZ is sqrt(X*X + Y*Y + Z*Z) = sqrt(dot(A, A)). */
		return std::sqrt(dot(v, v));
	}
	/** Returns a dist between two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T dist(const basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		return magn(l - r);
	}
	/** Returns a normalized copy of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> norm(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_norm(result.data, v.data);
		else
			detail::vector_norm(result.data, v.data);
		return result;
	}

	/** Converts a degree angle vector to radian angle vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> rad(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_rad(result.data, v.data);
		else
			detail::vector_rad(result.data, v.data);
		return result;
	}
	/** Converts a radian angle vector to degree angle vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> deg(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_deg(result.data, v.data);
		else
			detail::vector_deg(result.data, v.data);
		return result;
	}

	/** Produces a new vector which is the result of shuffling elements of another vector.
	 * @tparam I Indices of elements of the source vector in the order they should be shuffled to the destination vector.
	 * @return Result vector who's elements are specified by `I`. */
	template<std::size_t... I, typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vector<U, sizeof...(I), Sp> shuffle(const basic_vector<U, M, Sp> &l) noexcept
	{
		basic_vector<U, sizeof...(I)> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_shuffle(result.data, l.data, std::index_sequence<I...>{});
		else
			detail::vector_shuffle(result.data, l.data, std::index_sequence<I...>{});
		return result;
	}

	/** Gets the Ith element of the vector. */
	template<std::size_t I, typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T &get(basic_vector<T, N, Sp> &v) noexcept
	{
		return v.data.template get<I>();
	}
	/** @copydoc get */
	template<std::size_t I, typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr const T &get(const basic_vector<T, N, Sp> &v) noexcept
	{
		return v.data.template get<I>();
	}
}	 // namespace sek::math

template<typename T, std::size_t N, sek::math::storage_policy Sp>
struct std::tuple_size<sek::math::basic_vector<T, N, Sp>> : std::integral_constant<std::size_t, N>
{
};
template<std::size_t I, typename T, std::size_t N, sek::math::storage_policy Sp>
struct std::tuple_element<I, sek::math::basic_vector<T, N, Sp>>
{
	using type = T;
};
