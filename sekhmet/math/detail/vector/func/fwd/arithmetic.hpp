/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_ARITHMETIC                                                                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> operator+(const basic_vec<U, M, Q> &) noexcept                               \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> operator-(const basic_vec<U, M, Q> &) noexcept                               \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> abs(const basic_vec<U, M, Q> &) noexcept                                     \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> operator+(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> &operator+=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> operator-(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> &operator-=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> operator*(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> &operator*=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> operator/(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> &operator/=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> operator%(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> &operator%=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                  \
	friend constexpr basic_vec<U, M, Q> fmod(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;      \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> fmadd(                                                                        \
		const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &c) noexcept;            \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> fmsub(                                                                        \
		const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &c) noexcept;

namespace sek::math
{
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator+(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator+=(basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator-(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator-=(basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;

	// clang-format off
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator+(const basic_vec<U, M, Q> &v) noexcept requires std::is_signed_v<U>;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator-(const basic_vec<U, M, Q> &v) noexcept requires std::is_signed_v<U>;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> abs(const basic_vec<U, M, Q> &v) noexcept requires std::is_signed_v<U>;
	// clang-format on

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator*(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator*=(basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator/(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator/=(basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator*(const basic_vec<U, M, Q> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator*(U l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator*=(basic_vec<U, M, Q> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator/(const basic_vec<U, M, Q> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator/(U l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator/=(basic_vec<U, M, Q> &l, U r) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator%(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> operator%=(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator%(const basic_vec<U, M, Q> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> operator%=(const basic_vec<U, M, Q> &l, U r) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fmod(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fmod(const basic_vec<U, M, Q> &l, U r) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q>
		fmadd(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &c) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q>
		fmsub(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &c) noexcept;
}	 // namespace sek::math
