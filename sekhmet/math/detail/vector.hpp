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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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
	detail::vector_data_t<T, N, P> m_data = {};                                                                                 \
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
	: m_data(other.m_data)                                                                                                      \
	{                                                                                                                           \
	}                                                                                                                           \
                                                                                                                                \
	constexpr explicit basic_vector(const value_type(&vals)[N]) noexcept : m_data(vals)                                         \
	{                                                                                                                           \
	}                                                                                                                           \
                                                                                                                                \
	[[nodiscard]] constexpr value_type &operator[](std::size_t i) noexcept                                                      \
	{                                                                                                                           \
		return m_data[i];                                                                                                       \
	}                                                                                                                           \
	[[nodiscard]] constexpr const value_type &operator[](std::size_t i) const noexcept                                          \
	{                                                                                                                           \
		return m_data[i];                                                                                                       \
	}                                                                                                                           \
	constexpr void swap(basic_vector &other) noexcept                                                                           \
	{                                                                                                                           \
		m_data.swap(other.m_data);                                                                                              \
	}

#define SEK_VECTOR_PERMUTATION_FUNC_2(T, P, x, y)                                                                      \
	[[nodiscard]] constexpr basic_vector<T, 2, P> x##y() const noexcept                                                \
	{                                                                                                                  \
		return {x(), y()};                                                                                             \
	}
#define SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, y, z)                                                                   \
	[[nodiscard]] constexpr basic_vector<T, 3, P> x##y##z() const noexcept                                             \
	{                                                                                                                  \
		return {x(), y(), z()};                                                                                        \
	}
#define SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, z, w)                                                                \
	[[nodiscard]] constexpr basic_vector<T, 4, P> x##y##z##w() const noexcept                                          \
	{                                                                                                                  \
		return {x(), y(), z(), w()};                                                                                   \
	}

#define SEK_VECTOR_PERMUTATIONS_2(T, P, x, y)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, x, x)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, x, y)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, y, x)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, y, y)

#define SEK_VECTOR_PERMUTATIONS_3(T, P, x, y, z)                                                                       \
	SEK_VECTOR_PERMUTATIONS_2(T, P, x, y)                                                                              \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, x, z)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, y, z)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, z, x)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, z, y)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_2(T, P, z, z)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, x, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, x, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, x, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, y, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, y, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, y, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, z, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, z, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, z, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, x, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, x, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, x, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, y, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, y, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, y, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, z, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, z, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, z, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, x, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, x, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, x, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, y, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, y, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, y, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, z, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, z, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, z, z)

#define SEK_VECTOR_PERMUTATIONS_4(T, P, x, y, z, w)                                                                    \
	SEK_VECTOR_PERMUTATIONS_3(T, P, x, y, z)                                                                           \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, x, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, y, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, z, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, x, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, y, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, z, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, x, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, y, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, z, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, w, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, w, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, w, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, x, w, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, w, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, w, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, w, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, y, w, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, w, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, w, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, w, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, z, w, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, x, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, x, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, x, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, x, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, y, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, y, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, y, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, y, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, z, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, z, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, z, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, z, w)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, w, x)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, w, y)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, w, z)                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_3(T, P, w, w, w)                                                                       \
                                                                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, x, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, y, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, z, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, x, w, w, w)                                                                    \
                                                                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, x, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, y, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, z, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, y, w, w, w)                                                                    \
                                                                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, x, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, y, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, z, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, z, w, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, x, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, y, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, z, w, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, x, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, x, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, x, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, x, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, y, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, y, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, y, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, y, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, z, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, z, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, z, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, z, w)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, w, x)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, w, y)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, w, z)                                                                    \
	SEK_VECTOR_PERMUTATION_FUNC_4(T, P, w, w, w, w)

#define SEK_VECTOR_PERMUTATIONS(T, P, x, ...)                                                                          \
	SEK_GET_MACRO_3(__VA_ARGS__, SEK_VECTOR_PERMUTATIONS_4, SEK_VECTOR_PERMUTATIONS_3, SEK_VECTOR_PERMUTATIONS_2)      \
	(T, P, x, __VA_ARGS__)

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
		: m_data({data...})
		{
		}
	};

	template<arithmetic T, storage_policy Policy>
	union basic_vector<T, 2, Policy>
	{
		template<arithmetic U, std::size_t M, storage_policy P>
		friend union basic_vector;

		SEK_MATH_VECTOR_COMMON(T, 2, Policy)

		constexpr basic_vector(T x, T y) noexcept : m_data({x, y}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}) {}

		[[nodiscard]] constexpr T &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return m_data[1]; }

		SEK_VECTOR_PERMUTATIONS(T, Policy, x, y)
	};

	template<arithmetic T, storage_policy Policy>
	union basic_vector<T, 3, Policy>
	{
		template<arithmetic U, std::size_t M, storage_policy P>
		friend union basic_vector;

		SEK_MATH_VECTOR_COMMON(T, 3, Policy)

		constexpr basic_vector(T x, T y, T z) noexcept : m_data({x, y, z}) {}
		constexpr basic_vector(T x, T y) noexcept : basic_vector(x, y, T{}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}, T{}) {}

		[[nodiscard]] constexpr T &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &z() const noexcept { return m_data[2]; }

		SEK_VECTOR_PERMUTATIONS(T, Policy, x, y, z)

		[[nodiscard]] constexpr T &r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &b() const noexcept { return m_data[2]; }

		SEK_VECTOR_PERMUTATIONS(T, Policy, r, g, b)
	};

	template<arithmetic T, storage_policy Policy>
	union basic_vector<T, 4, Policy>
	{
		template<arithmetic U, std::size_t M, storage_policy P>
		friend union basic_vector;

		SEK_MATH_VECTOR_COMMON(T, 4, Policy)

		constexpr basic_vector(T x, T y, T z, T w) noexcept : m_data({x, y, z, w}) {}
		constexpr basic_vector(T x, T y, T z) noexcept : basic_vector(x, y, z, T{}) {}
		constexpr basic_vector(T x, T y) noexcept : basic_vector(x, y, T{}, T{}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}, T{}, T{}) {}

		[[nodiscard]] constexpr T &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &z() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr T &w() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const T &w() const noexcept { return m_data[3]; }

		SEK_VECTOR_PERMUTATIONS(T, Policy, x, y, z, w)

		[[nodiscard]] constexpr T &r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &b() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr T &a() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const T &a() const noexcept { return m_data[3]; }

		SEK_VECTOR_PERMUTATIONS(T, Policy, r, g, b, a)

		[[nodiscard]] constexpr T &s() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &s() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &t() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &t() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &p() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &p() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr T &q() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const T &q() const noexcept { return m_data[3]; }

		SEK_VECTOR_PERMUTATIONS(T, Policy, s, t, p, q)
	};

	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr sek::hash_t hash(const basic_vector<T, N, Sp> &v) noexcept
	{
		return v.m_data.hash();
	}
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr void swap(basic_vector<T, N, Sp> &a, basic_vector<T, N, Sp> &b) noexcept
	{
		a.swap(b);
	}

	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr auto operator<=>(const basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		return l.m_data <=> r.m_data;
	}
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr bool operator==(const basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		return l.m_data == r.m_data;
	}

	/** Returns a vector which is the result of addition of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator+(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_add(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Adds a vector to a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator+=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_add(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of subtraction of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator-(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Subtracts a vector from a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator-=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(l.m_data, l.m_data, r.m_data);
		return l;
	}

	/** Returns a copy of a vector multiplied by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator*(const basic_vector<T, N, Sp> &l, T r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(result.m_data, l.m_data, r);
		else
			detail::vector_mul(result.m_data, l.m_data, r);
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
			detail::generic::vector_mul(l.m_data, l.m_data, r);
		else
			detail::vector_mul(l.m_data, l.m_data, r);
		return l;
	}
	/** Returns a copy of a vector divided by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator/(const basic_vector<T, N, Sp> &l, T r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.m_data, l.m_data, r);
		else
			detail::vector_div(result.m_data, l.m_data, r);
		return result;
	}
	/** Returns a vector produced by dividing a scalar by a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator/(T l, const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.m_data, l, r.m_data);
		else
			detail::vector_div(result.m_data, l, r.m_data);
		return result;
	}
	/** Divides vector by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator/=(basic_vector<T, N, Sp> &l, T r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_div(l.m_data, l.m_data, r);
		else
			detail::vector_div(l.m_data, l.m_data, r);
		return l;
	}

	/** Preforms a bitwise AND on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator&=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_and(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise AND of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator&(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_and(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise OR on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator|=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_or(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise OR of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator|(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_or(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Returns a vector which is the result of bitwise XOR of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator^(const basic_vector<T, N, Sp> &l,
															 const basic_vector<T, N, Sp> &r) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise XOR on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vector<T, N, Sp> &operator^=(basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a bitwise inverted copy of a vector. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> operator~(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_inv(result.m_data, v.m_data);
		else
			detail::vector_inv(result.m_data, v.m_data);
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
			detail::generic::vector_neg(result.m_data, v.m_data);
		else
			detail::vector_neg(result.m_data, v.m_data);
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
			detail::generic::vector_abs(result.m_data, v.m_data);
		else
			detail::vector_abs(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of maximum data of a and b.
	 * @example max({0, 1, 3}, {-1, 2, 2}) -> {0, 2, 3} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> max(const basic_vector<T, N, Sp> &a, const basic_vector<T, N, Sp> &b) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_max(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_max(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Returns a vector consisting of minimum data of a and b.
	 * @example min({0, 1, 3}, {-1, 2, 2}) -> {-1, 1, 2} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> min(const basic_vector<T, N, Sp> &a, const basic_vector<T, N, Sp> &b) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_min(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_min(result.m_data, a.m_data, b.m_data);
		return result;
	}

	/** Calculates square root of a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> sqrt(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sqrt(result.m_data, v.m_data);
		else
			detail::vector_sqrt(result.m_data, v.m_data);
		return result;
	}
	/** Calculates reciprocal square root of a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> rsqrt(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_rsqrt(result.m_data, v.m_data);
		else
			detail::vector_rsqrt(result.m_data, v.m_data);
		return result;
	}

	/** Calculates dot product of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T dot(const basic_vector<T, N, Sp> &l, const basic_vector<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::generic::vector_dot(l.m_data, r.m_data);
		else
			return detail::vector_dot(l.m_data, r.m_data);
	}
	/** Calculates cross product of two vectors. */
	template<typename T, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, 3, Sp> cross(const basic_vector<T, 3, Sp> &l, const basic_vector<T, 3, Sp> &r) noexcept
		requires std::is_signed_v<T>
	{
		basic_vector<T, 3> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cross(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_cross(result.m_data, l.m_data, r.m_data);
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
			detail::generic::vector_norm(result.m_data, v.m_data);
		else
			detail::vector_norm(result.m_data, v.m_data);
		return result;
	}

	/** Converts a degree angle vector to radian angle vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> rad(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_rad(result.m_data, v.m_data);
		else
			detail::vector_rad(result.m_data, v.m_data);
		return result;
	}
	/** Converts a radian angle vector to degree angle vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vector<T, N, Sp> deg(const basic_vector<T, N, Sp> &v) noexcept
	{
		basic_vector<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_deg(result.m_data, v.m_data);
		else
			detail::vector_deg(result.m_data, v.m_data);
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
			detail::generic::vector_shuffle(result.m_data, l.m_data, std::index_sequence<I...>{});
		else
			detail::vector_shuffle(result.m_data, l.m_data, std::index_sequence<I...>{});
		return result;
	}

	/** Gets the Ith element of the vector. */
	template<std::size_t I, typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T &get(basic_vector<T, N, Sp> &v) noexcept
	{
		return v.m_data.template get<I>();
	}
	/** @copydoc get */
	template<std::size_t I, typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr const T &get(const basic_vector<T, N, Sp> &v) noexcept
	{
		return v.m_data.template get<I>();
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
