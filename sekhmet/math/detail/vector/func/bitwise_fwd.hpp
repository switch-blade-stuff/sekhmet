/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../fwd.hpp"

#define SEK_DETAIL_FRIEND_BITWISE                                                                                    \
	template<std::integral U, std::size_t M, policy_t Sp>                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator&=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
	template<std::integral U, std::size_t M, policy_t Sp>                                                        \
	friend constexpr basic_vec<U, M, Sp> operator&(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<std::integral U, std::size_t M, policy_t Sp>                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator|=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
	template<std::integral U, std::size_t M, policy_t Sp>                                                        \
	friend constexpr basic_vec<U, M, Sp> operator|(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<std::integral U, std::size_t M, policy_t Sp>                                                        \
	friend constexpr basic_vec<U, M, Sp> operator^(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<std::integral U, std::size_t M, policy_t Sp>                                                        \
	friend constexpr basic_vec<U, M, Sp> &operator^=(basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;     \
	template<std::integral U, std::size_t M, policy_t Sp>                                                        \
	friend constexpr basic_vec<U, M, Sp> operator~(const basic_vec<U, M, Sp> &) noexcept;

namespace sek::math
{
	template<std::integral U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator&=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator&(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator|=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator|(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator^(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator^=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<std::integral U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator~(const basic_vec<U, M, Sp> &v) noexcept;
}	 // namespace sek::math
