//
// Created by switchblade on 2021-12-16.
//

#pragma once

#include <numbers>

#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/hash.hpp"
#include "util.hpp"
#include "vector_data.hpp"
#include "vectorfwd.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/vector_impl.hpp"
#else
#warning "SMID vector operations are not implemented for this CPU"
#define SEK_NO_SIMD
#endif
#endif

#include "generic/vector_impl.hpp"

/* Use a macro to avoid duplicating common definitions for vector specializations. */
#define SEK_MATH_VECTOR_COMMON(T, N)                                                                                   \
private:                                                                                                               \
	detail::vector_data_t<T, N> data = {};                                                                             \
                                                                                                                       \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr sek::hash_t hash(const basic_vector<U, M> &) noexcept;                                            \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr void swap(basic_vector<U, M> &, basic_vector<U, M> &) noexcept;                                   \
                                                                                                                       \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr auto operator<=>(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;                \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr bool operator==(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;                 \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> operator+(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;    \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> &operator+=(basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;        \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> operator-(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;    \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> &operator-=(basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;        \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> operator*(const basic_vector<U, M> &, U) noexcept;                             \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> &operator*=(basic_vector<U, M> &, U) noexcept;                                 \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> operator/(const basic_vector<U, M> &, U) noexcept;                             \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> &operator/=(basic_vector<U, M> &, U) noexcept;                                 \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr basic_vector<U, M> &operator&=(basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;        \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr basic_vector<U, M> operator&(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;    \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr basic_vector<U, M> &operator|=(basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;        \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr basic_vector<U, M> operator|(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;    \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr basic_vector<U, M> operator^(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;    \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr basic_vector<U, M> &operator^=(basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;        \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr basic_vector<U, M> operator~(const basic_vector<U, M> &) noexcept;                                \
                                                                                                                       \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> operator+(const basic_vector<U, M> &) noexcept                                 \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> operator-(const basic_vector<U, M> &) noexcept                                 \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> abs(const basic_vector<U, M> &) noexcept                                       \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> max(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;          \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> min(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;          \
                                                                                                                       \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> sqrt(const basic_vector<U, M> &) noexcept;                                     \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> rsqrt(const basic_vector<U, M> &) noexcept;                                    \
                                                                                                                       \
	template<typename U>                                                                                               \
	friend constexpr basic_vector<U, 3> cross(const basic_vector<U, 3> &, const basic_vector<U, 3> &) noexcept         \
		requires std::is_signed_v<U>                                                                                   \
	;                                                                                                                  \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr U dot(const basic_vector<U, M> &, const basic_vector<U, M> &) noexcept;                           \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr basic_vector<U, M> norm(const basic_vector<U, M> &) noexcept;                                     \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr U magn(const basic_vector<U, M> &) noexcept;                                                      \
                                                                                                                       \
	template<std::size_t I, typename U, std::size_t M>                                                                 \
	friend constexpr U &get(basic_vector<U, M> &) noexcept;                                                            \
	template<std::size_t I, typename U, std::size_t M>                                                                 \
	friend constexpr const U &get(const basic_vector<U, M> &) noexcept;                                                \
                                                                                                                       \
public:                                                                                                                \
	typedef T value_type;                                                                                              \
	constexpr basic_vector() noexcept = default;                                                                       \
	constexpr explicit basic_vector(const value_type(&vals)[N]) noexcept : data(vals)                                  \
	{                                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	[[nodiscard]] constexpr value_type &operator[](std::size_t i) noexcept                                             \
	{                                                                                                                  \
		SEK_ASSERT(i < N);                                                                                             \
		return data[i];                                                                                                \
	}                                                                                                                  \
	[[nodiscard]] constexpr const value_type &operator[](std::size_t i) const noexcept                                 \
	{                                                                                                                  \
		SEK_ASSERT(i < N);                                                                                             \
		return data[i];                                                                                                \
	}                                                                                                                  \
	template<std::size_t M>                                                                                            \
	[[nodiscard]] constexpr operator basic_vector<value_type, M>() const noexcept                                      \
	{                                                                                                                  \
		return vector_cast<M>(*this);                                                                                  \
	}                                                                                                                  \
	constexpr void swap(basic_vector &other) noexcept                                                                  \
	{                                                                                                                  \
		data.swap(other.data);                                                                                         \
	}

namespace sek::math
{
	/** Generic vector overload.
	 * @note Generic vector types are not guaranteed to have SIMD-optimized. */
	template<arithmetic T, std::size_t N>
	union basic_vector
	{
		SEK_MATH_VECTOR_COMMON(T, N)

	private:
		template<typename... U>
		constexpr static bool compatible_type_seq = std::conjunction_v<std::is_convertible<U, T>...>;

	public:
		template<arithmetic... U>
		constexpr explicit basic_vector(U... data) noexcept
			requires compatible_type_seq<U...>
		: data({data...})
		{
		}
	};

	template<arithmetic T>
	union basic_vector<T, 2>
	{
		SEK_MATH_VECTOR_COMMON(T, 2)

		constexpr basic_vector(T x, T y) noexcept : data({x, y}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}) {}

#ifndef SEK_NO_ANONYMOUS_STRUCT
		struct
		{
			T x;
			T y;
		};
#endif
	};

	template<arithmetic T>
	union basic_vector<T, 3>
	{
		SEK_MATH_VECTOR_COMMON(T, 3)

		constexpr basic_vector(T x, T y, T z) noexcept : data({x, y, z}) {}
		constexpr basic_vector(T x, T y) noexcept : basic_vector(x, y, T{}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}, T{}) {}

#ifndef SEK_NO_ANONYMOUS_STRUCT
		struct
		{
			T x;
			T y;
			T z;
		};
		struct
		{
			T r;
			T g;
			T b;
		};
		struct
		{
			T s;
			T t;
			T p;
		};
#endif
	};

	template<arithmetic T>
	union basic_vector<T, 4>
	{
		SEK_MATH_VECTOR_COMMON(T, 4)

		constexpr basic_vector(T x, T y, T z, T w) noexcept : data({x, y, z, w}) {}
		constexpr basic_vector(T x, T y, T z) noexcept : basic_vector(x, y, z, T{}) {}
		constexpr basic_vector(T x, T y) noexcept : basic_vector(x, y, T{}, T{}) {}
		constexpr explicit basic_vector(T x) noexcept : basic_vector(x, T{}, T{}, T{}) {}

#ifndef SEK_NO_ANONYMOUS_STRUCT
		struct
		{
			T x;
			T y;
			T z;
			T w;
		};
		struct
		{
			T r;
			T g;
			T b;
			T a;
		};
		struct
		{
			T s;
			T t;
			T p;
			T q;
		};
#endif
	};

	template<std::size_t N, std::size_t M, typename T>
	[[nodiscard]] constexpr basic_vector<T, N> vector_cast(const basic_vector<T, M> &src) noexcept
	{
		constexpr auto make_vector = []<std::size_t... Is>(std::index_sequence<Is...>, const basic_vector<T, M> &src)
		{
			constexpr auto extract = []<std::size_t I>(const basic_vector<T, M> &src)
			{
				if constexpr (I > M)
					return T{};
				else
					return src[I];
			};

			return basic_vector<T, N>{{extract<Is>(src)...}};
		};
		return make_vector(std::make_index_sequence<N>(), src);
	}

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr sek::hash_t hash(const basic_vector<T, N> &v) noexcept
	{
		return v.data.hash();
	}
	template<typename T, std::size_t N>
	constexpr void swap(basic_vector<T, N> &a, basic_vector<T, N> &b) noexcept
	{
		a.swap(b);
	}

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr auto operator<=>(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		return l.data <=> r.data;
	}
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr bool operator==(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		return l.data == r.data;
	}

	/** Returns a vector which is the result of addition of two vectors. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator+(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_add(result.data, l.data, r.data);
		else
			detail::vector_add(result.data, l.data, r.data);
		return result;
	}
	/** Adds a vector to a vector. */
	template<typename T, std::size_t N>
	constexpr basic_vector<T, N> &operator+=(basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_add(l.data, l.data, r.data);
		else
			detail::vector_add(l.data, l.data, r.data);
		return l;
	}
	/** Returns a vector which is the result of subtraction of two vectors. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator-(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(result.data, l.data, r.data);
		else
			detail::vector_sub(result.data, l.data, r.data);
		return result;
	}
	/** Subtracts a vector from a vector. */
	template<typename T, std::size_t N>
	constexpr basic_vector<T, N> &operator-=(basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_sub(l.data, l.data, r.data);
		else
			detail::vector_sub(l.data, l.data, r.data);
		return l;
	}

	/** Returns a copy of the vector multiplied by a scalar. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator*(const basic_vector<T, N> &l, T r) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(result.data, l.data, r);
		else
			detail::vector_mul(result.data, l.data, r);
		return result;
	}
	/** @copydoc operator* */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator*(T l, const basic_vector<T, N> &r) noexcept
	{
		return r * l;
	}
	/** Multiplies vector by a scalar. */
	template<typename T, std::size_t N>
	constexpr basic_vector<T, N> &operator*=(basic_vector<T, N> &l, T r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_mul(l.data, l.data, r);
		else
			detail::vector_mul(l.data, l.data, r);
		return l;
	}
	/** Returns a copy of the vector divided by a scalar. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator/(const basic_vector<T, N> &l, T r) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_div(result.data, l.data, r);
		else
			detail::vector_div(result.data, l.data, r);
		return result;
	}
	/** Divides vector by a scalar. */
	template<typename T, std::size_t N>
	constexpr basic_vector<T, N> &operator/=(basic_vector<T, N> &l, T r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_div(l.data, l.data, r);
		else
			detail::vector_div(l.data, l.data, r);
		return l;
	}

	/** Preforms a bitwise AND on two vectors. */
	template<std::integral T, std::size_t N>
	constexpr basic_vector<T, N> &operator&=(basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_and(l.data, l.data, r.data);
		else
			detail::vector_and(l.data, l.data, r.data);
		return l;
	}
	/** Returns a vector which is the result of bitwise AND of two vectors. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator&(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_and(result.data, l.data, r.data);
		else
			detail::vector_and(result.data, l.data, r.data);
		return result;
	}
	/** Preforms a bitwise OR on two vectors. */
	template<std::integral T, std::size_t N>
	constexpr basic_vector<T, N> &operator|=(basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_or(l.data, l.data, r.data);
		else
			detail::vector_or(l.data, l.data, r.data);
		return l;
	}
	/** Returns a vector which is the result of bitwise OR of two vectors. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator|(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_or(result.data, l.data, r.data);
		else
			detail::vector_or(result.data, l.data, r.data);
		return result;
	}
	/** Returns a vector which is the result of bitwise XOR of two vectors. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator^(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(result.data, l.data, r.data);
		else
			detail::vector_xor(result.data, l.data, r.data);
		return result;
	}
	/** Preforms a bitwise XOR on two vectors. */
	template<std::integral T, std::size_t N>
	constexpr basic_vector<T, N> &operator^=(basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(l.data, l.data, r.data);
		else
			detail::vector_xor(l.data, l.data, r.data);
		return l;
	}
	/** Returns a bitwise inverted copy of a vector. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator~(const basic_vector<T, N> &v) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_inv(result.data, v.data);
		else
			detail::vector_inv(result.data, v.data);
		return result;
	}

	/** Returns a copy of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator+(const basic_vector<T, N> &v) noexcept
		requires std::is_signed_v<T>
	{
		return v;
	}
	/** Returns a negated copy of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator-(const basic_vector<T, N> &v) noexcept
		requires std::is_signed_v<T>
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_neg(result.data, v.data);
		else
			detail::vector_neg(result.data, v.data);
		return result;
	}
	/** Calculates absolute value of a vector.
	 * @example abs({-1, 2, 0}) -> {1, 2, 0} */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> abs(const basic_vector<T, N> &v) noexcept
		requires std::is_signed_v<T>
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_abs(result.data, v.data);
		else
			detail::vector_abs(result.data, v.data);
		return result;
	}
	/** Returns a vector consisting of maximum data of a and b.
	 * @example max({0, 1, 3}, {-1, 2, 2}) -> {0, 2, 3} */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> max(const basic_vector<T, N> &a, const basic_vector<T, N> &b) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_max(result.data, a.data, b.data);
		else
			detail::vector_max(result.data, a.data, b.data);
		return result;
	}
	/** Returns a vector consisting of minimum data of a and b.
	 * @example min({0, 1, 3}, {-1, 2, 2}) -> {-1, 1, 2} */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> min(const basic_vector<T, N> &a, const basic_vector<T, N> &b) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_min(result.data, a.data, b.data);
		else
			detail::vector_min(result.data, a.data, b.data);
		return result;
	}

	/** Calculates square root of a vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> sqrt(const basic_vector<T, N> &v) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sqrt(result.data, v.data);
		else
			detail::vector_sqrt(result.data, v.data);
		return result;
	}
	/** Calculates reciprocal square root of a vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> rsqrt(const basic_vector<T, N> &v) noexcept
	{
		basic_vector<T, N> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_rsqrt(result.data, v.data);
		else
			detail::vector_rsqrt(result.data, v.data);
		return result;
	}

	/** Calculates dot product of two vectors. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr T dot(const basic_vector<T, N> &l, const basic_vector<T, N> &r) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::generic::vector_dot(l.data, r.data);
		else
			return detail::vector_dot(l.data, r.data);
	}
	/** Calculates cross product of two vectors. */
	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 3> cross(const basic_vector<T, 3> &l, const basic_vector<T, 3> &r) noexcept
		requires std::is_signed_v<T>
	{
		basic_vector<T, 3> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cross(result.data, l.data, r.data);
		else
			detail::vector_cross(result.data, l.data, r.data);
		return result;
	}
	/** Returns a length of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr T magn(const basic_vector<T, N> &v) noexcept
	{
		/* Magnitude of a vector A=XYZ is sqrt(X*X + Y*Y + Z*Z) = sqrt(dot(A, A)). */
		return std::sqrt(dot(v, v));
	}
	/** Returns a normalized copy of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> norm(const basic_vector<T, N> &v) noexcept
	{
		basic_vector<T, N> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_norm(result.data, v.data);
		else
			detail::vector_norm(result.data, v.data);
		return result;
	}

	/** Gets the Ith element of the vector. */
	template<std::size_t I, typename T, std::size_t N>
	[[nodiscard]] constexpr T &get(basic_vector<T, N> &v) noexcept
	{
		return v.data.template get<I>();
	}
	/** Gets the Ith element of the vector. */
	template<std::size_t I, typename T, std::size_t N>
	[[nodiscard]] constexpr const T &get(const basic_vector<T, N> &v) noexcept
	{
		return v.data.template get<I>();
	}
}	 // namespace sek::math

template<typename T, std::size_t N>
struct std::tuple_size<sek::math::basic_vector<T, N>> : std::integral_constant<std::size_t, N>
{
};
template<std::size_t I, typename T, std::size_t N>
struct std::tuple_element<I, sek::math::basic_vector<T, N>>
{
	using type = T;
};
