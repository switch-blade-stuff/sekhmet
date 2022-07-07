/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_BITWISE                                                                                    \
	template<std::integral U, std::size_t M, policy_t Q>                                                        \
	friend constexpr basic_vec<U, M, Q> &operator&=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
	template<std::integral U, std::size_t M, policy_t Q>                                                        \
	friend constexpr basic_vec<U, M, Q> operator&(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<std::integral U, std::size_t M, policy_t Q>                                                        \
	friend constexpr basic_vec<U, M, Q> &operator|=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
	template<std::integral U, std::size_t M, policy_t Q>                                                        \
	friend constexpr basic_vec<U, M, Q> operator|(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<std::integral U, std::size_t M, policy_t Q>                                                        \
	friend constexpr basic_vec<U, M, Q> operator^(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<std::integral U, std::size_t M, policy_t Q>                                                        \
	friend constexpr basic_vec<U, M, Q> &operator^=(basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;     \
	template<std::integral U, std::size_t M, policy_t Q>                                                        \
	friend constexpr basic_vec<U, M, Q> operator~(const basic_vec<U, M, Q> &) noexcept;

namespace sek::math
{
	template<std::integral U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator&=(basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator&(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator|=(basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator|(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator^(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Q>
	constexpr basic_vec<U, M, Q> &operator^=(basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> operator~(const basic_vec<U, M, Q> &v) noexcept;
}	 // namespace sek::math
