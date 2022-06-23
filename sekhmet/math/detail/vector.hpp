/*
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include "sekhmet/detail/hash.hpp"

#include "macros.hpp"
#include "util.hpp"
#include "vector_data.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/vector_impl.hpp"
#else
#warning "SMID vector operations are not implemented for this CPU"
#define SEK_NO_SIMD
#endif
#endif

#include "generic/vector_impl.hpp"

namespace sek::math
{
	/** @brief Structure representing a mathematical vector.
	 * @tparam T Type of values stored in the vector.
	 * @tparam N Amount of values the vector holds.
	 * @tparam Policy Policy used for storage & optimization. */
	template<typename T, std::size_t N, storage_policy Policy = storage_policy::OPTIMAL>
	class basic_vec;

	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 2, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_MATH_VECTOR_COMMON(T, 2, Policy)

	public:
		constexpr basic_vec(T x, T y) noexcept : m_data({x, y}) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x) {}

		[[nodiscard]] constexpr T &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return m_data[1]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y)
	};
	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 3, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_MATH_VECTOR_COMMON(T, 3, Policy)

	public:
		constexpr basic_vec(T x, T y, T z) noexcept : m_data({x, y, z}) {}
		constexpr basic_vec(T x, T y) noexcept : basic_vec(x, y, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x, x) {}

		[[nodiscard]] constexpr T &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &z() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z)

		[[nodiscard]] constexpr T &r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &b() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b)
	};
	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 4, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_MATH_VECTOR_COMMON(T, 4, Policy)

	public:
		constexpr basic_vec(T x, T y, T z, T w) noexcept : m_data({x, y, z, w}) {}
		constexpr basic_vec(T x, T y, T z) noexcept : basic_vec(x, y, z, z) {}
		constexpr basic_vec(T x, T y) noexcept : basic_vec(x, y, y, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x, x, x) {}

		[[nodiscard]] constexpr T &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &z() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr T &w() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const T &w() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z, w)

		[[nodiscard]] constexpr T &r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &b() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr T &a() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const T &a() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b, a)

		[[nodiscard]] constexpr T &s() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const T &s() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr T &t() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const T &t() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr T &p() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const T &p() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr T &q() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const T &q() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(s, t, p, q)
	};
	template<storage_policy Policy>
	class basic_vec<bool, 2, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_MATH_VECTOR_COMMON(bool, 2, Policy)

	public:
		constexpr basic_vec(bool x, bool y) noexcept : basic_vec(x, y) {}
		constexpr explicit basic_vec(bool x) noexcept : basic_vec(x, x) {}

		[[nodiscard]] constexpr bool &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const bool &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr bool &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const bool &y() const noexcept { return m_data[1]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y)

		/** Converts the bool vector to a boolean by ANDing all elements. */
		[[nodiscard]] constexpr operator bool() const noexcept;
	};
	template<storage_policy Policy>
	class basic_vec<bool, 3, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_MATH_VECTOR_COMMON(bool, 3, Policy)

	public:
		constexpr basic_vec(bool x, bool y, bool z) noexcept : basic_vec(x, y, z) {}
		constexpr basic_vec(bool x, bool y) noexcept : basic_vec(x, y, y) {}
		constexpr explicit basic_vec(bool x) noexcept : basic_vec(x, x, x) {}

		[[nodiscard]] constexpr bool &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const bool &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr bool &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const bool &y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr bool &z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const bool &z() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z)

		[[nodiscard]] constexpr bool &r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const bool &r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr bool &g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const bool &g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr bool &b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const bool &b() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b)

		/** Converts the bool vector to a boolean by ANDing all elements. */
		[[nodiscard]] constexpr operator bool() const noexcept;
	};
	template<storage_policy Policy>
	class basic_vec<bool, 4, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_MATH_VECTOR_COMMON(bool, 4, Policy)

	public:
		constexpr basic_vec(bool x, bool y, bool z, bool w) noexcept : m_data({x, y, z, w}) {}
		constexpr basic_vec(bool x, bool y, bool z) noexcept : basic_vec(x, y, z, z) {}
		constexpr basic_vec(bool x, bool y) noexcept : basic_vec(x, y, y, y) {}
		constexpr explicit basic_vec(bool x) noexcept : basic_vec(x, x, x, x) {}

		[[nodiscard]] constexpr bool &x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const bool &x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr bool &y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const bool &y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr bool &z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const bool &z() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr bool &w() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const bool &w() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z, w)

		[[nodiscard]] constexpr bool &r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const bool &r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr bool &g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const bool &g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr bool &b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const bool &b() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr bool &a() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const bool &a() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b, a)

		[[nodiscard]] constexpr bool &s() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr const bool &s() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr bool &t() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr const bool &t() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr bool &p() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr const bool &p() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr bool &q() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr const bool &q() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(s, t, p, q)

		/** Converts the bool vector to a boolean by ANDing all elements. */
		[[nodiscard]] constexpr operator bool() const noexcept;
	};

	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr sek::hash_t hash(const basic_vec<T, N, Sp> &v) noexcept
	{
		return v.m_data.hash();
	}
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr void swap(basic_vec<T, N, Sp> &a, basic_vec<T, N, Sp> &b) noexcept
	{
		a.swap(b);
	}

	/** Returns a vector which is the result of addition of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator+(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_add(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Adds a vector to a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator+=(basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_add(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_add(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of subtraction of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator-(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Subtracts a vector from a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator-=(basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_sub(l.m_data, l.m_data, r.m_data);
		return l;
	}

	/** Returns a copy of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator+(const basic_vec<T, N, Sp> &v) noexcept
		requires std::is_signed_v<T>
	{
		return v;
	}
	/** Returns a negated copy of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator-(const basic_vec<T, N, Sp> &v) noexcept
		requires std::is_signed_v<T>
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_neg(result.m_data, v.m_data);
		else
			detail::vector_neg(result.m_data, v.m_data);
		return result;
	}

	/** Returns a copy of a vector multiplied by another vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator*(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_mul(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Multiplies vector by another vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator*=(basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_mul(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a copy of a vector divided by another vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator/(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_div(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Divides vector by another vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator/=(basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_div(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a copy of a vector multiplied by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator*(const basic_vec<T, N, Sp> &l, T r) noexcept
	{
		return l * basic_vec<T, N, Sp>{r};
	}
	/** @copydoc operator* */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator*(T l, const basic_vec<T, N, Sp> &r) noexcept
	{
		return l * basic_vec<T, N, Sp>{r};
	}
	/** Multiplies vector by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator*=(basic_vec<T, N, Sp> &l, T r) noexcept
	{
		return l *= basic_vec<T, N, Sp>{r};
	}
	/** Returns a copy of a vector divided by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator/(const basic_vec<T, N, Sp> &l, T r) noexcept
	{
		return l / basic_vec<T, N, Sp>{r};
	}
	/** Returns a vector produced by dividing a scalar by a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator/(T l, const basic_vec<T, N, Sp> &r) noexcept
	{
		return basic_vec<T, N, Sp>{l} / r;
	}
	/** Divides vector by a scalar. */
	template<typename T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator/=(basic_vec<T, N, Sp> &l, T r) noexcept
	{
		return l /= basic_vec<T, N, Sp>{r};
	}

	/** Calculates modulus of two vectors. */
	template<typename U, std::size_t M, storage_policy Sp>
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
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mod(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_mod(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Calculates modulus of vector and a scalar. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator%(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l % basic_vec<U, M, Sp>{r};
	}
	/** @copydoc operator% */
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> operator%=(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return l %= basic_vec<U, M, Sp>{r};
	}
	/** Calculates floating-point modulus of two vectors. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
		requires std::floating_point<U>
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_fmod(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_fmod(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Calculates floating-point modulus of vector and a scalar. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmod(const basic_vec<U, M, Sp> &l, U r) noexcept
		requires std::floating_point<U>
	{
		return fmod(l, basic_vec<U, M, Sp>{r});
	}

	/** Calculates absolute value of a vector.
	 * @example abs({-1, 2, 0}) -> {1, 2, 0} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> abs(const basic_vec<T, N, Sp> &v) noexcept
		requires std::is_signed_v<T>
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_abs(result.m_data, v.m_data);
		else
			detail::vector_abs(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of maximum data of a and b.
	 * @example max({0, 1, 3}, {-1, 2, 2}) -> {0, 2, 3} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> max(const basic_vec<T, N, Sp> &a, const basic_vec<T, N, Sp> &b) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_max(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_max(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Returns a vector consisting of minimum data of a and b.
	 * @example min({0, 1, 3}, {-1, 2, 2}) -> {-1, 1, 2} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> min(const basic_vec<T, N, Sp> &a, const basic_vec<T, N, Sp> &b) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_min(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_min(result.m_data, a.m_data, b.m_data);
		return result;
	}

	/** Returns a vector consisting of rounded values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> round(const basic_vec<T, N, Sp> &v) noexcept
		requires std::floating_point<T>
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_round(result.m_data, v.m_data);
		else
			detail::vector_round(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of rounded-down values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> floor(const basic_vec<T, N, Sp> &v) noexcept
		requires std::floating_point<T>
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_floor(result.m_data, v.m_data);
		else
			detail::vector_floor(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of rounded-up values of `v`.
	 * @example round({.1, .2, 2.3}) -> {1, 1, 3} */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> ceil(const basic_vec<T, N, Sp> &v) noexcept
		requires std::floating_point<T>
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_ceil(result.m_data, v.m_data);
		else
			detail::vector_ceil(result.m_data, v.m_data);
		return result;
	}

	// clang-format off
	/** Calculates linear interpolation or extrapolation between two vectors. Equivalent to `l + t * (r - l)`. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, const basic_vec<U, M, Sp> &t) noexcept
	{
		return l + t * (r - l);
	}
	/** @copydoc lerp */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, U t) noexcept
	{
		return lerp(l, r, basic_vec<U, M, Sp>{t});
	}
	// clang-format on

	/** Returns a vector of `e` raised to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_exp(result.m_data, v.m_data);
		else
			detail::vector_exp(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector of `2` raised to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp2(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_exp2(result.m_data, v.m_data);
		else
			detail::vector_exp2(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector of `e` raised to the given power, minus one. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> expm1(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_expm1(result.m_data, v.m_data);
		else
			detail::vector_expm1(result.m_data, v.m_data);
		return result;
	}
	/** Calculates natural logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log(result.m_data, v.m_data);
		else
			detail::vector_log(result.m_data, v.m_data);
		return result;
	}
	/** Calculates common logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log10(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log10(result.m_data, v.m_data);
		else
			detail::vector_log10(result.m_data, v.m_data);
		return result;
	}
	/** Calculates base-2 logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log2(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log2(result.m_data, v.m_data);
		else
			detail::vector_log2(result.m_data, v.m_data);
		return result;
	}
	/** Calculates natural logarithms of 1 plus a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log1p(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log1p(result.m_data, v.m_data);
		else
			detail::vector_log1p(result.m_data, v.m_data);
		return result;
	}

	/** Raises elements of a vector to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_pow(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_pow(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** @copydoc pow */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return pow(l, basic_vec<U, M, Sp>{r});
	}
	/** Calculates square root of a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> sqrt(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sqrt(result.m_data, v.m_data);
		else
			detail::vector_sqrt(result.m_data, v.m_data);
		return result;
	}
	/** Calculates cubic root of a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> cbrt(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cbrt(result.m_data, v.m_data);
		else
			detail::vector_cbrt(result.m_data, v.m_data);
		return result;
	}
	/** Calculates reciprocal square root of a vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> rsqrt(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_rsqrt(result.m_data, v.m_data);
		else
			detail::vector_rsqrt(result.m_data, v.m_data);
		return result;
	}

	/** Preforms a bitwise AND on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator&=(basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_and(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise AND of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator&(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_and(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise OR on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator|=(basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_or(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise OR of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator|(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_or(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Returns a vector which is the result of bitwise XOR of two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator^(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise XOR on two vectors. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	constexpr basic_vec<T, N, Sp> &operator^=(basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a bitwise inverted copy of a vector. */
	template<std::integral T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> operator~(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_inv(result.m_data, v.m_data);
		else
			detail::vector_inv(result.m_data, v.m_data);
		return result;
	}

	/** Calculates dot product of two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T dot(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::generic::vector_dot(l.m_data, r.m_data);
		else
			return detail::vector_dot(l.m_data, r.m_data);
	}
	/** Calculates cross product of two vectors. */
	template<typename T, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, 3, Sp> cross(const basic_vec<T, 3, Sp> &l, const basic_vec<T, 3, Sp> &r) noexcept
		requires std::is_signed_v<T>
	{
		basic_vec<T, 3> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cross(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_cross(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Returns a length of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T magn(const basic_vec<T, N, Sp> &v) noexcept
	{
		/* Magnitude of a vector A=XYZ is sqrt(X*X + Y*Y + Z*Z) = sqrt(dot(A, A)). */
		return std::sqrt(dot(v, v));
	}
	/** Returns a dist between two vectors. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T dist(const basic_vec<T, N, Sp> &l, const basic_vec<T, N, Sp> &r) noexcept
	{
		return magn(l - r);
	}
	/** Returns a normalized copy of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> norm(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_norm(result.m_data, v.m_data);
		else
			detail::vector_norm(result.m_data, v.m_data);
		return result;
	}

	/** Converts a degree angle vector to radian angle vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> rad(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_rad(result.m_data, v.m_data);
		else
			detail::vector_rad(result.m_data, v.m_data);
		return result;
	}
	/** Converts a radian angle vector to degree angle vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> deg(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_deg(result.m_data, v.m_data);
		else
			detail::vector_deg(result.m_data, v.m_data);
		return result;
	}

	/** Calculates a sine of the elements of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> sin(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_sin(result.m_data, v.m_data);
		else
			detail::vector_sin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a cosine of the elements of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> cos(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cos(result.m_data, v.m_data);
		else
			detail::vector_cos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a tangent of the elements of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> tan(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_tan(result.m_data, v.m_data);
		else
			detail::vector_tan(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc sine of the elements of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> asin(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_asin(result.m_data, v.m_data);
		else
			detail::vector_asin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc cosine of the elements of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> acos(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acos(result.m_data, v.m_data);
		else
			detail::vector_acos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc tangent of the elements of the vector. */
	template<typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, N, Sp> atan(const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_atan(result.m_data, v.m_data);
		else
			detail::vector_atan(result.m_data, v.m_data);
		return result;
	}

	/** Gets the Ith element of the vector. */
	template<std::size_t I, typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr T &get(basic_vec<T, N, Sp> &v) noexcept
	{
		return v.m_data.template get<I>();
	}
	/** @copydoc get */
	template<std::size_t I, typename T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr const T &get(const basic_vec<T, N, Sp> &v) noexcept
	{
		return v.m_data.template get<I>();
	}

	namespace detail
	{
		template<std::size_t... Is, typename T, std::size_t N, storage_policy Sp, typename F>
		constexpr void vectorize_impl(std::index_sequence<Is...>, const basic_vec<T, N, Sp> &v, F &&f)
		{
			(f(get<Is>(v)), ...);
		}
		template<std::size_t... Is, typename T, std::size_t N, storage_policy Sp, typename F>
		constexpr void vectorize_impl(std::index_sequence<Is...>, basic_vec<T, N, Sp> &v, F &&f)
		{
			(f(get<Is>(v)), ...);
		}
	}	 // namespace detail

	/** Applies a functor to every element of the vector. */
	template<typename T, std::size_t N, storage_policy Sp, typename F>
	constexpr void vectorize(const basic_vec<T, N, Sp> &v, F &&f)
	{
		detail::vectorize_impl(std::make_index_sequence<N>{}, v, std::forward<F>(f));
	}
	/** @copydoc vectorize */
	template<typename T, std::size_t N, storage_policy Sp, typename F>
	constexpr void vectorize(basic_vec<T, N, Sp> &v, F &&f)
	{
		detail::vectorize_impl(std::make_index_sequence<N>{}, v, std::forward<F>(f));
	}

	/** Produces a new vector which is the result of shuffling elements of another vector.
	 * @tparam I Indices of elements of the source vector in the order they should be shuffled to the destination vector.
	 * @return Result vector who's elements are specified by `I`.
	 * @example shuffle<2, 1, 0>({3, 4, 5}) -> {5, 4, 3} */
	template<std::size_t... I, typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, sizeof...(I), Sp> shuffle(const basic_vec<U, M, Sp> &v) noexcept
	{
		using Idx = std::index_sequence<I...>;
		if constexpr (std::is_same_v<Idx, std::make_index_sequence<M>>)
			return v;
		else
		{
			basic_vec<U, sizeof...(I)> result;
			if (std::is_constant_evaluated())
				detail::generic::vector_shuffle(result.m_data, v.m_data, Idx{});
			else
				detail::vector_shuffle(result.m_data, v.m_data, Idx{});
			return result;
		}
	}

	/** Checks if all components of the vector are `true`. */
	template<std::convertible_to<bool> T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr bool all(const basic_vec<T, N, Sp> &v) noexcept
	{
		bool result = true;
		vectorize(v, [&result](T value) noexcept { result = result && static_cast<bool>(value); });
		return result;
	}
	/** Checks if any components of the vector are `true`. */
	template<std::convertible_to<bool> T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr bool any(const basic_vec<T, N, Sp> &v) noexcept
	{
		bool result = false;
		vectorize(v, [&result](T value) noexcept { result = result || static_cast<bool>(value); });
		return result;
	}
	/** Checks if no components of the vector are `true`. */
	template<std::convertible_to<bool> T, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr bool none(const basic_vec<T, N, Sp> &v) noexcept
	{
		return !any(v);
	}

	template<storage_policy Sp>
	constexpr basic_vec<bool, 2, Sp>::operator bool() const noexcept
	{
		return all(*this);
	}
	template<storage_policy Sp>
	constexpr basic_vec<bool, 3, Sp>::operator bool() const noexcept
	{
		return all(*this);
	}
	template<storage_policy Sp>
	constexpr basic_vec<bool, 4, Sp>::operator bool() const noexcept
	{
		return all(*this);
	}

	template<typename U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> operator==(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_eq(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_eq(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> operator!=(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ne(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ne(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> operator<(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_lt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_lt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> operator<=(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_le(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_le(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> operator>(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_gt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_gt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> operator>=(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ge(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ge(result.m_data, l.m_data, r.m_data);
		return result;
	}

	template<std::integral U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> &operator&&(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
		requires std::convertible_to<U, bool>
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_and_bool(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_and_bool(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<std::integral U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> &operator||(const basic_vec<U, N, Sp> &l, const basic_vec<U, N, Sp> &r) noexcept
		requires std::convertible_to<U, bool>
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_or_bool(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_or_bool(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<std::integral U, std::size_t N, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<bool, N, Sp> &operator!(const basic_vec<U, N, Sp> &v) noexcept
		requires std::convertible_to<U, bool>
	{
		basic_vec<bool, N, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_neg_bool(result.m_data, v.m_data);
		else
			detail::vector_neg_bool(result.m_data, v.m_data);
		return result;
	}
}	 // namespace sek::math

template<typename T, std::size_t N, sek::math::storage_policy Sp>
struct std::tuple_size<sek::math::basic_vec<T, N, Sp>> : std::integral_constant<std::size_t, N>
{
};
template<std::size_t I, typename T, std::size_t N, sek::math::storage_policy Sp>
struct std::tuple_element<I, sek::math::basic_vec<T, N, Sp>>
{
	using type = T;
};
