/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_EXPONENTIAL                                                                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> exp(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> exp2(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> expm1(const basic_vec<U, M, Q> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> log(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> log10(const basic_vec<U, M, Q> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> log2(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> log1p(const basic_vec<U, M, Q> &) noexcept;                                  \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> pow(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> sqrt(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> rsqrt(const basic_vec<U, M, Q> &) noexcept;

namespace sek::math
{
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> exp(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> exp2(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> expm1(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> log(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> log10(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> log2(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> log1p(const basic_vec<U, M, Q> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> pow(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> pow(const basic_vec<U, M, Q> &l, U r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> sqrt(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> cbrt(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> rsqrt(const basic_vec<U, M, Q> &v) noexcept;
}	 // namespace sek::math
