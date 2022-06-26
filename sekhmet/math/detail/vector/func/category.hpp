/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../fwd.hpp"

#define SEK_DETAIL_FRIEND_CATEGORY                                                                            \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_nan(const basic_vec<U, M, Sp> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_inf(const basic_vec<U, M, Sp> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_fin(const basic_vec<U, M, Sp> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_neg(const basic_vec<U, M, Sp> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_norm(const basic_vec<U, M, Sp> &) noexcept;

namespace sek::math
{
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_nan(const basic_vec<U, M, Sp> &v) noexcept;
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_inf(const basic_vec<U, M, Sp> &v) noexcept;
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_fin(const basic_vec<U, M, Sp> &v) noexcept;
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_neg(const basic_vec<U, M, Sp> &v) noexcept;
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_norm(const basic_vec<U, M, Sp> &v) noexcept;
}	 // namespace sek::math
