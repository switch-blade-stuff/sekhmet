/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_TRIGONOMETRIC                                                                                \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> sin(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> cos(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> tan(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> cot(const basic_vec<U, M, Sp> &) noexcept;                                    \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> asin(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> acos(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> atan(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> acot(const basic_vec<U, M, Sp> &) noexcept;                                   \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> sinh(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> cosh(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> tanh(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> coth(const basic_vec<U, M, Sp> &) noexcept;                                   \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> asinh(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> acosh(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> atanh(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> acoth(const basic_vec<U, M, Sp> &) noexcept;                                  \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> rad(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> deg(const basic_vec<U, M, Sp> &) noexcept;

namespace sek::math
{
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sin(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cos(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tan(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cot(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asin(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acos(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atan(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acot(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sinh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cosh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tanh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> coth(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asinh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acosh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atanh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acoth(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> rad(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> deg(const basic_vec<U, M, Sp> &v) noexcept;
}	 // namespace sek::math
