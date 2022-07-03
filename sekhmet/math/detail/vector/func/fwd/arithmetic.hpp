/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_ARITHMETIC                                                                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &) noexcept                               \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &) noexcept                               \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> abs(const basic_vec<U, M, Sp> &) noexcept                                     \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> &operator+=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> &operator-=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> &operator%=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
	template<std::floating_point U, std::size_t M, policy_t Sp>                                                  \
	friend constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;      \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fmadd(                                                                        \
		const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &c) noexcept;            \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fmsub(                                                                        \
		const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &c) noexcept;

namespace sek::math
{
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator+=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator-=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;

	// clang-format off
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &v) noexcept requires std::is_signed_v<U>;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &v) noexcept requires std::is_signed_v<U>;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> abs(const basic_vec<U, M, Sp> &v) noexcept requires std::is_signed_v<U>;
	// clang-format on

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(U l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(U l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &l, U r) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, U r) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, U r) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmadd(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &c) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmsub(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &c) noexcept;
}	 // namespace sek::math
