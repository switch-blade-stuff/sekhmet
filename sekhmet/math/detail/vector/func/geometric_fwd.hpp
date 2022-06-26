/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../fwd.hpp"

#define SEK_DETAIL_FRIEND_GEOMETRIC                                                                                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> lerp(                                                                         \
		const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;               \
	template<typename U, storage_policy Sp>                                                                            \
	friend constexpr basic_vec<U, 3, Sp> cross(const basic_vec<U, 3, Sp> &, const basic_vec<U, 3, Sp> &) noexcept      \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr U dot(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                         \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> norm(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr U magn(const basic_vec<U, M, Sp> &) noexcept;

namespace sek::math
{
	// clang-format off
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, const basic_vec<U, M, Sp> &t) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, U t) noexcept;
	template<typename T, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, 3, Sp> cross(const basic_vec<T, 3, Sp> &l, const basic_vec<T, 3, Sp> &r) noexcept requires std::is_signed_v<T>;
	// clang-format on

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr U dot(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr U magn(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> norm(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr U dist(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept;
}	 // namespace sek::math
