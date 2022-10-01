/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_TRIGONOMETRIC                                                                                \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> sin(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> cos(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> tan(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> cot(const basic_vec<U, M, Q> &) noexcept;                                    \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> asin(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> acos(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> atan(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> acot(const basic_vec<U, M, Q> &) noexcept;                                   \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> sinh(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> cosh(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> tanh(const basic_vec<U, M, Q> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> coth(const basic_vec<U, M, Q> &) noexcept;                                   \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> asinh(const basic_vec<U, M, Q> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> acosh(const basic_vec<U, M, Q> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> atanh(const basic_vec<U, M, Q> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> acoth(const basic_vec<U, M, Q> &) noexcept;                                  \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> rad(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Q>                                                             \
	friend constexpr basic_vec<U, M, Q> deg(const basic_vec<U, M, Q> &) noexcept;

namespace sek
{
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> sin(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> cos(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> tan(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> cot(const basic_vec<U, M, Q> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> asin(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acos(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> atan(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acot(const basic_vec<U, M, Q> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> sinh(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> cosh(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> tanh(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> coth(const basic_vec<U, M, Q> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> asinh(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acosh(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> atanh(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acoth(const basic_vec<U, M, Q> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> rad(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> deg(const basic_vec<U, M, Q> &v) noexcept;
}	 // namespace sek
