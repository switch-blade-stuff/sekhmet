/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../fwd.hpp"

#define SEK_DETAIL_FRIEND_EXPONENTIAL                                                                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> exp(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> exp2(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> expm1(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> log(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> log10(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> log2(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> log1p(const basic_vec<U, M, Sp> &) noexcept;                                  \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> sqrt(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> rsqrt(const basic_vec<U, M, Sp> &) noexcept;

namespace sek::math
{
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp2(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> expm1(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log10(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log2(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log1p(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sqrt(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cbrt(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> rsqrt(const basic_vec<U, M, Sp> &v) noexcept;
}	 // namespace sek::math
