/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_RELATIONAL                                                                                            \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator==(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator!=(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator<(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;  \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator<=(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator>(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;  \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator>=(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept; \
                                                                                                                                  \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator==(const vec_mask<basic_vec<U, M, Q>> &,                              \
															  const vec_mask<basic_vec<U, M, Q>> &) noexcept;                    \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator!=(const vec_mask<basic_vec<U, M, Q>> &,                              \
															  const vec_mask<basic_vec<U, M, Q>> &) noexcept;                    \
                                                                                                                                  \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator&&(const vec_mask<basic_vec<U, M, Q>> &,                              \
															  const vec_mask<basic_vec<U, M, Q>> &) noexcept;                    \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator||(const vec_mask<basic_vec<U, M, Q>> &,                              \
															  const vec_mask<basic_vec<U, M, Q>> &) noexcept;                    \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Q>> operator!(const vec_mask<basic_vec<U, M, Q>> &) noexcept;                     \
                                                                                                                                  \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr basic_vec<U, M, Q> max(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;                  \
	template<typename U, std::size_t M, policy_t Q>                                                                        \
	friend constexpr basic_vec<U, M, Q> std::min(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;

namespace sek::math
{
	template<std::convertible_to<bool> U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool all(const vec_mask<basic_vec<U, M, Q>> &m) noexcept;
	template<std::convertible_to<bool> U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool any(const vec_mask<basic_vec<U, M, Q>> &m) noexcept;
	template<std::convertible_to<bool> U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool none(const vec_mask<basic_vec<U, M, Q>> &m) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator==(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator!=(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator&&(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator||(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator!(const vec_mask<basic_vec<U, M, Q>> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator==(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator!=(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator<(const basic_vec<U, M, Q> &l,
																	const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator<=(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator>(const basic_vec<U, M, Q> &l,
																	const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator>=(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept;

	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_eq(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> fcmp_eq(const basic_vec<U, M, Q> &a,
																  const basic_vec<U, M, Q> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_ne(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> fcmp_ne(const basic_vec<U, M, Q> &a,
																  const basic_vec<U, M, Q> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_le(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> fcmp_le(const basic_vec<U, M, Q> &a,
																  const basic_vec<U, M, Q> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_ge(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> fcmp_ge(const basic_vec<U, M, Q> &a,
																  const basic_vec<U, M, Q> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_lt(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> fcmp_lt(const basic_vec<U, M, Q> &a,
																  const basic_vec<U, M, Q> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_gt(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> fcmp_gt(const basic_vec<U, M, Q> &a,
																  const basic_vec<U, M, Q> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> max(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> std::min(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> clamp(const basic_vec<U, M, Q> &value,
													  const basic_vec<U, M, Q> &min_val,
													  const basic_vec<U, M, Q> &max_val) noexcept;

	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q>
		fstd::min(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fstd::min(const basic_vec<U, M, Q> &a,
													 const basic_vec<U, M, Q> &b,
													 U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q>
		fmax(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fmax(const basic_vec<U, M, Q> &a,
													 const basic_vec<U, M, Q> &b,
													 U epsilon = std::numeric_limits<U>::epsilon()) noexcept;

	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fclamp(const basic_vec<U, M, Q> &value,
													   const basic_vec<U, M, Q> &min_val,
													   const basic_vec<U, M, Q> &max_val,
													   const basic_vec<U, M, Q> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fclamp(const basic_vec<U, M, Q> &value,
													   const basic_vec<U, M, Q> &min_val,
													   const basic_vec<U, M, Q> &max_val,
													   U = std::numeric_limits<U>::epsilon()) noexcept;

}	 // namespace sek::math