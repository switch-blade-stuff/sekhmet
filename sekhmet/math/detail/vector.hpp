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

#define SEK_VECTOR_PERMUTATION_FUNC(T, P, name, ...)                                                                   \
	[[nodiscard]] constexpr auto name() const noexcept                                                                 \
	{                                                                                                                  \
		return shuffle<__VA_ARGS__>(*this);                                                                            \
	}

#define SEK_VECTOR_PERMUTATIONS_2(T, P, x, y)                                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x), 0, 0)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y), 0, 1)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x), 1, 0)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y), 1, 1)

#define SEK_VECTOR_PERMUTATIONS_3(T, P, x, y, z)                                                                       \
	SEK_VECTOR_PERMUTATIONS_2(T, P, x, y)                                                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z), 0, 2)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z), 1, 2)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x), 2, 0)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y), 2, 1)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z), 2, 2)                                                          \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, x), 0, 0, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, y), 0, 0, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, z), 0, 0, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, x), 0, 1, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, y), 0, 1, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, z), 0, 1, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, x), 0, 2, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, y), 0, 2, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, z), 0, 2, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, x), 1, 0, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, y), 1, 0, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, z), 1, 0, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, x), 1, 1, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, y), 1, 1, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, z), 1, 1, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, x), 1, 2, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, y), 1, 2, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, z), 1, 2, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, x), 2, 0, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, y), 2, 0, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, z), 2, 0, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, x), 2, 1, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, y), 2, 1, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, z), 2, 1, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, x), 2, 2, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, y), 2, 2, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, z), 2, 2, 2)

#define SEK_VECTOR_PERMUTATIONS_4(T, P, x, y, z, w)                                                                    \
	SEK_VECTOR_PERMUTATIONS_3(T, P, x, y, z)                                                                           \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, w), 0, 0, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, w), 0, 1, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, w), 0, 2, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, w), 1, 0, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, w), 1, 1, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, w), 1, 2, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, w), 2, 0, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, w), 2, 1, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, w), 2, 2, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, x), 0, 3, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, y), 0, 3, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, z), 0, 3, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, w), 0, 3, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, x), 1, 3, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, y), 1, 3, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, z), 1, 3, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, w), 1, 3, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, x), 2, 3, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, y), 2, 3, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, z), 2, 3, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, w), 2, 3, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, x), 3, 0, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, y), 3, 0, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, z), 3, 0, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, w), 3, 0, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, x), 3, 1, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, y), 3, 1, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, z), 3, 1, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, w), 3, 1, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, x), 3, 2, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, y), 3, 2, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, z), 3, 2, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, w), 3, 2, 3)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, x), 3, 3, 0)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, y), 3, 3, 1)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, z), 3, 3, 2)                                                    \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, w), 3, 3, 3)                                                    \
                                                                                                                       \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, x, x), 0, 0, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, x, y), 0, 0, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, x, z), 0, 0, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, x, w), 0, 0, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, y, x), 0, 0, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, y, y), 0, 0, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, y, z), 0, 0, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, y, w), 0, 0, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, z, x), 0, 0, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, z, y), 0, 0, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, z, z), 0, 0, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, z, w), 0, 0, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, w, x), 0, 0, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, w, y), 0, 0, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, w, z), 0, 0, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, x, w, w), 0, 0, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, x, x), 0, 1, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, x, y), 0, 1, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, x, z), 0, 1, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, x, w), 0, 1, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, y, x), 0, 1, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, y, y), 0, 1, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, y, z), 0, 1, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, y, w), 0, 1, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, z, x), 0, 1, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, z, y), 0, 1, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, z, z), 0, 1, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, z, w), 0, 1, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, w, x), 0, 1, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, w, y), 0, 1, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, w, z), 0, 1, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, y, w, w), 0, 1, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, x, x), 0, 2, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, x, y), 0, 2, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, x, z), 0, 2, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, x, w), 0, 2, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, y, x), 0, 2, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, y, y), 0, 2, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, y, z), 0, 2, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, y, w), 0, 2, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, z, x), 0, 2, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, z, y), 0, 2, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, z, z), 0, 2, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, z, w), 0, 2, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, w, x), 0, 2, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, w, y), 0, 2, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, w, z), 0, 2, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, z, w, w), 0, 2, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, x, x), 0, 3, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, x, y), 0, 3, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, x, z), 0, 3, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, x, w), 0, 3, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, y, x), 0, 3, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, y, y), 0, 3, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, y, z), 0, 3, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, y, w), 0, 3, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, z, x), 0, 3, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, z, y), 0, 3, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, z, z), 0, 3, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, z, w), 0, 3, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, w, x), 0, 3, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, w, y), 0, 3, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, w, z), 0, 3, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(x, w, w, w), 0, 3, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, x, x), 1, 0, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, x, y), 1, 0, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, x, z), 1, 0, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, x, w), 1, 0, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, y, x), 1, 0, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, y, y), 1, 0, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, y, z), 1, 0, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, y, w), 1, 0, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, z, x), 1, 0, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, z, y), 1, 0, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, z, z), 1, 0, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, z, w), 1, 0, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, w, x), 1, 0, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, w, y), 1, 0, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, w, z), 1, 0, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, x, w, w), 1, 0, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, x, x), 1, 1, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, x, y), 1, 1, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, x, z), 1, 1, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, x, w), 1, 1, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, y, x), 1, 1, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, y, y), 1, 1, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, y, z), 1, 1, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, y, w), 1, 1, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, z, x), 1, 1, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, z, y), 1, 1, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, z, z), 1, 1, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, z, w), 1, 1, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, w, x), 1, 1, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, w, y), 1, 1, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, w, z), 1, 1, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, y, w, w), 1, 1, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, x, x), 1, 2, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, x, y), 1, 2, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, x, z), 1, 2, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, x, w), 1, 2, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, y, x), 1, 2, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, y, y), 1, 2, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, y, z), 1, 2, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, y, w), 1, 2, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, z, x), 1, 2, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, z, y), 1, 2, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, z, z), 1, 2, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, z, w), 1, 2, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, w, x), 1, 2, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, w, y), 1, 2, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, w, z), 1, 2, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, z, w, w), 1, 2, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, x, x), 1, 3, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, x, y), 1, 3, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, x, z), 1, 3, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, x, w), 1, 3, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, y, x), 1, 3, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, y, y), 1, 3, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, y, z), 1, 3, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, y, w), 1, 3, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, z, x), 1, 3, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, z, y), 1, 3, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, z, z), 1, 3, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, z, w), 1, 3, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, w, x), 1, 3, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, w, y), 1, 3, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, w, z), 1, 3, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(y, w, w, w), 1, 3, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, x, x), 2, 0, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, x, y), 2, 0, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, x, z), 2, 0, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, x, w), 2, 0, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, y, x), 2, 0, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, y, y), 2, 0, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, y, z), 2, 0, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, y, w), 2, 0, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, z, x), 2, 0, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, z, y), 2, 0, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, z, z), 2, 0, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, z, w), 2, 0, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, w, x), 2, 0, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, w, y), 2, 0, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, w, z), 2, 0, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, x, w, w), 2, 0, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, x, x), 2, 1, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, x, y), 2, 1, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, x, z), 2, 1, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, x, w), 2, 1, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, y, x), 2, 1, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, y, y), 2, 1, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, y, z), 2, 1, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, y, w), 2, 1, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, z, x), 2, 1, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, z, y), 2, 1, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, z, z), 2, 1, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, z, w), 2, 1, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, w, x), 2, 1, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, w, y), 2, 1, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, w, z), 2, 1, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, y, w, w), 2, 1, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, x, x), 2, 2, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, x, y), 2, 2, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, x, z), 2, 2, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, x, w), 2, 2, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, y, x), 2, 2, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, y, y), 2, 2, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, y, z), 2, 2, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, y, w), 2, 2, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, z, x), 2, 2, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, z, y), 2, 2, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, z, z), 2, 2, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, z, w), 2, 2, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, w, x), 2, 2, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, w, y), 2, 2, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, w, z), 2, 2, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, z, w, w), 2, 2, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, x, x), 2, 3, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, x, y), 2, 3, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, x, z), 2, 3, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, x, w), 2, 3, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, y, x), 2, 3, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, y, y), 2, 3, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, y, z), 2, 3, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, y, w), 2, 3, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, z, x), 2, 3, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, z, y), 2, 3, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, z, z), 2, 3, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, z, w), 2, 3, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, w, x), 2, 3, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, w, y), 2, 3, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, w, z), 2, 3, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(z, w, w, w), 2, 3, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, x, x), 3, 0, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, x, y), 3, 0, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, x, z), 3, 0, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, x, w), 3, 0, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, y, x), 3, 0, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, y, y), 3, 0, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, y, z), 3, 0, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, y, w), 3, 0, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, z, x), 3, 0, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, z, y), 3, 0, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, z, z), 3, 0, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, z, w), 3, 0, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, w, x), 3, 0, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, w, y), 3, 0, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, w, z), 3, 0, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, x, w, w), 3, 0, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, x, x), 3, 1, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, x, y), 3, 1, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, x, z), 3, 1, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, x, w), 3, 1, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, y, x), 3, 1, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, y, y), 3, 1, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, y, z), 3, 1, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, y, w), 3, 1, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, z, x), 3, 1, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, z, y), 3, 1, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, z, z), 3, 1, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, z, w), 3, 1, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, w, x), 3, 1, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, w, y), 3, 1, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, w, z), 3, 1, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, y, w, w), 3, 1, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, x, x), 3, 2, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, x, y), 3, 2, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, x, z), 3, 2, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, x, w), 3, 2, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, y, x), 3, 2, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, y, y), 3, 2, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, y, z), 3, 2, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, y, w), 3, 2, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, z, x), 3, 2, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, z, y), 3, 2, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, z, z), 3, 2, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, z, w), 3, 2, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, w, x), 3, 2, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, w, y), 3, 2, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, w, z), 3, 2, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, z, w, w), 3, 2, 3, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, x, x), 3, 3, 0, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, x, y), 3, 3, 0, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, x, z), 3, 3, 0, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, x, w), 3, 3, 0, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, y, x), 3, 3, 1, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, y, y), 3, 3, 1, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, y, z), 3, 3, 1, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, y, w), 3, 3, 1, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, z, x), 3, 3, 2, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, z, y), 3, 3, 2, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, z, z), 3, 3, 2, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, z, w), 3, 3, 2, 3)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, w, x), 3, 3, 3, 0)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, w, y), 3, 3, 3, 1)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, w, z), 3, 3, 3, 2)                                              \
	SEK_VECTOR_PERMUTATION_FUNC(T, P, SEK_CONCAT(w, w, w, w), 3, 3, 3, 3)

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
