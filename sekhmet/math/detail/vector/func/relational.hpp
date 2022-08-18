/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/relational.hpp"
#endif
#endif

namespace sek::math
{
	namespace detail
	{
		inline namespace generic
		{
			template<typename T, std::size_t N, policy_t P>
			constexpr void mask_eq(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] == r[i];
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void mask_ne(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] != r[i];
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void mask_and(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<bool>(l[i] && r[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void mask_or(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<bool>(l[i] || r[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void mask_neg(mask_data<T, N, P> &out, const mask_data<T, N, P> &m) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = !static_cast<bool>(m[i]);
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void
				vector_cmp(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r, auto p) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = p(l[i], r[i]);
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_eq(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				vector_cmp(out, l, r, [](T a, T b) { return a == b; });
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_ne(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				vector_cmp(out, l, r, [](T a, T b) { return a != b; });
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_lt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				vector_cmp(out, l, r, [](T a, T b) { return a < b; });
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_le(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				vector_cmp(out, l, r, [](T a, T b) { return a <= b; });
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_gt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				vector_cmp(out, l, r, [](T a, T b) { return a > b; });
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_ge(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				vector_cmp(out, l, r, [](T a, T b) { return a >= b; });
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_max(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				mask_data<T, N, P> mask;
				vector_ge(mask, l, r);
				vector_interleave(out, l, r, mask);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_min(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				mask_data<T, N, P> mask;
				vector_le(mask, l, r);
				vector_interleave(out, l, r, mask);
			}
		}	 // namespace generic
	}		 // namespace detail

	/** Checks if all components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool all(const vec_mask<basic_vec<U, M, Q>> &m) noexcept
	{
		bool result = true;
		for (std::size_t i = 0; i < M; ++i) result = result && static_cast<bool>(m[i]);
		return result;
	}
	/** Checks if any components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool any(const vec_mask<basic_vec<U, M, Q>> &m) noexcept
	{
		bool result = true;
		for (std::size_t i = 0; i < M; ++i) result = result || static_cast<bool>(m[i]);
		return result;
	}
	/** Checks if no components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool none(const vec_mask<basic_vec<U, M, Q>> &m) noexcept
	{
		return !any(m);
	}

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator==(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_eq(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_eq(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator!=(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_ne(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_ne(result.m_data, l.m_data, r.m_data);
		return result;
	}

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator&&(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_and(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_and(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator||(const vec_mask<basic_vec<U, M, Q>> &l,
																	 const vec_mask<basic_vec<U, M, Q>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_or(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_or(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator!(const vec_mask<basic_vec<U, M, Q>> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_neg(result.m_data, v.m_data);
		else
			detail::mask_neg(result.m_data, v.m_data);
		return result;
	}

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator==(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_eq(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_eq(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator!=(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ne(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ne(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator<(const basic_vec<U, M, Q> &l,
																	const basic_vec<U, M, Q> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_lt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_lt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator<=(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_le(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_le(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator>(const basic_vec<U, M, Q> &l,
																	const basic_vec<U, M, Q> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_gt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_gt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> operator>=(const basic_vec<U, M, Q> &l,
																	 const basic_vec<U, M, Q> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ge(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ge(result.m_data, l.m_data, r.m_data);
		return result;
	}

	/** Checks if elements of vector a equals vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_eq(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return a == b || abs(a - b) <= epsilon;
	}
	/** @copydoc fcmp_eq */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_eq(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fcmp_eq(a, b, basic_vec<U, M, Q>{epsilon});
	}
	/** Checks if elements of vector a does not equal vector vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_ne(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return a != b || abs(a - b) > epsilon;
	}
	/** @copydoc fcmp_ne */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_ne(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fcmp_ne(a, b, basic_vec<U, M, Q>{epsilon});
	}
	/** Checks if elements of vector a is less than or equal to vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_le(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return a <= b || abs(a - b) <= epsilon;
	}
	/** @copydoc fcmp_le */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_le(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fcmp_le(a, b, basic_vec<U, M, Q>{epsilon});
	}
	/** Checks if elements of vector a is greater than or equal to vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_ge(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return a >= b || abs(a - b) <= epsilon;
	}
	/** @copydoc fcmp_ge */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_ge(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fcmp_ge(a, b, basic_vec<U, M, Q>{epsilon});
	}
	/** Checks if elements of vector a is less than vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_lt(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return a < b && abs(a - b) > epsilon;
	}
	/** @copydoc fcmp_lt */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_lt(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fcmp_lt(a, b, basic_vec<U, M, Q>{epsilon});
	}
	/** Checks if elements of vector a is less than vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_gt(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return a > b && abs(a - b) > epsilon;
	}
	/** @copydoc fcmp_gt */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>>
		fcmp_gt(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fcmp_gt(a, b, basic_vec<U, M, Q>{epsilon});
	}

	/** Returns a vector consisting of maximum elements of a and b. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> max(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b) noexcept
	{
		basic_vec<U, M, Q> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_max(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_max(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Returns a vector consisting of minimum elements of a and b. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> min(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b) noexcept
	{
		basic_vec<U, M, Q> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_min(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_min(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Clamps elements of a vector between a minimum and a maximum. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q>
		clamp(const basic_vec<U, M, Q> &value, const basic_vec<U, M, Q> &min_val, const basic_vec<U, M, Q> &max_val) noexcept
	{
		return max(min_val, min(max_val, value));
	}

	/** Returns a vector consisting of minimum elements of a and b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q>
		fmin(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return interleave(a, b, fcmp_le(a, b, epsilon));
	}
	/** @copydoc fmin */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fmin(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fmin(a, b, basic_vec<U, M, Q>{epsilon});
	}
	/** Returns a vector consisting of maximum elements of a and b using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q>
		fmax(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return interleave(a, b, fcmp_ge(a, b, epsilon));
	}
	/** @copydoc fmax */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fmax(const basic_vec<U, M, Q> &a, const basic_vec<U, M, Q> &b, U epsilon) noexcept
	{
		return fmax(a, b, basic_vec<U, M, Q>{epsilon});
	}

	/** Clamps elements of a vector between a minimum and a maximum using an epsilon. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fclamp(const basic_vec<U, M, Q> &value,
													   const basic_vec<U, M, Q> &min_val,
													   const basic_vec<U, M, Q> &max_val,
													   const basic_vec<U, M, Q> &epsilon) noexcept
	{
		return fmax(min_val, fmin(max_val, value, epsilon), epsilon);
	}
	/** @copydoc fclamp */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> fclamp(const basic_vec<U, M, Q> &value,
													   const basic_vec<U, M, Q> &min_val,
													   const basic_vec<U, M, Q> &max_val,
													   U epsilon) noexcept
	{
		return fclamp(value, min_val, max_val, basic_vec<U, M, Q>{epsilon});
	}

}	 // namespace sek::math