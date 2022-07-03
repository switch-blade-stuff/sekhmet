/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/arithmetic.hpp"
#endif
#endif

namespace sek::math
{
	namespace detail
	{
		inline namespace generic
		{
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_add(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] + r[i];
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_sub(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] - r[i];
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_mul(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] * r[i];
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_div(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] / r[i];
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_mod(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] % r[i];
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_fmod(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::fmod(l[i], r[i]));
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_neg(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = -v[i];
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_abs(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::abs(v[i]));
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_fmadd(vector_data<T, N, P> &out,
										const vector_data<T, N, P> &a,
										const vector_data<T, N, P> &b,
										const vector_data<T, N, P> &c) noexcept
			{
				vector_mul(out, a, b);
				vector_add(out, out, c);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_fmsub(vector_data<T, N, P> &out,
										const vector_data<T, N, P> &a,
										const vector_data<T, N, P> &b,
										const vector_data<T, N, P> &c) noexcept
			{
				vector_mul(out, a, b);
				vector_sub(out, out, c);
			}
		}	 // namespace generic
	}		 // namespace detail

	/** Returns a vector which is the result of addition of two vectors. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_add(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Adds a vector to a vector. */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator+=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_add(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of subtraction of two vectors. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Subtracts a vector from a vector. */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator-=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(l.m_data, l.m_data, r.m_data);
		return l;
	}

	/** Returns a copy of the vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator+(const basic_vec<U, M, Sp> &v) noexcept
		requires std::is_signed_v<U>
	{
		return v;
	}
	/** Returns a negated copy of the vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator-(const basic_vec<U, M, Sp> &v) noexcept
		requires std::is_signed_v<U>
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_neg(result.m_data, v.m_data);
		else
			detail::vector_neg(result.m_data, v.m_data);
		return result;
	}

	/** Returns a copy of a vector multiplied by another vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_mul(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Multiplies vector by another vector. */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_mul(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a copy of a vector divided by another vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_div(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Divides vector by another vector. */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_div(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a copy of a vector multiplied by a scalar. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l * basic_vec<U, M, Sp>{r};
	}
	/** @copydoc operator* */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator*(U l, const basic_vec<U, M, Sp> &r) noexcept
	{
		return l * basic_vec<U, M, Sp>{r};
	}
	/** Multiplies vector by a scalar. */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator*=(basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l *= basic_vec<U, M, Sp>{r};
	}
	/** Returns a copy of a vector divided by a scalar. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l / basic_vec<U, M, Sp>{r};
	}
	/** Returns a vector produced by dividing a scalar by a vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator/(U l, const basic_vec<U, M, Sp> &r) noexcept
	{
		return basic_vec<U, M, Sp>{l} / r;
	}
	/** Divides vector by a scalar. */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> &operator/=(basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l /= basic_vec<U, M, Sp>{r};
	}

	/** Calculates modulus of two vectors. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mod(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_mod(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** @copydoc operator% */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mod(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_mod(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Calculates modulus of vector and a scalar. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l % basic_vec<U, M, Sp>{r};
	}
	/** @copydoc operator% */
	template<typename U, std::size_t M, policy_t Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l %= basic_vec<U, M, Sp>{r};
	}
	/** Calculates floating-point modulus of two vectors. */
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_fmod(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_fmod(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Calculates floating-point modulus of vector and a scalar. */
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return fmod(l, basic_vec<U, M, Sp>{r});
	}

	/** Calculates absolute value of a vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> abs(const basic_vec<U, M, Sp> &v) noexcept
		requires std::is_signed_v<U>
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_abs(result.m_data, v.m_data);
		else
			detail::vector_abs(result.m_data, v.m_data);
		return result;
	}

	/** Preforms a multiply-add operation on elements of vectors `a`, `b` and `c`. Equivalent to `(a * b) + c`. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmadd(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &c) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_fmadd(result.m_data, a.m_data, b.m_data, c.m_data);
		else
			detail::vector_fmadd(result.m_data, a.m_data, b.m_data, c.m_data);
		return result;
	}
	/** Preforms a multiply-subtract operation on elements of vectors `a`, `b` and `c`. Equivalent to `(a * b) - c`. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmsub(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &c) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_fmsub(result.m_data, a.m_data, b.m_data, c.m_data);
		else
			detail::vector_fmsub(result.m_data, a.m_data, b.m_data, c.m_data);
		return result;
	}
}	 // namespace sek::math
