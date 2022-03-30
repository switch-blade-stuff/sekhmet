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
	friend constexpr sek::hash_t hash<>(const vector<T, N> &) noexcept;                                                \
	friend constexpr void swap<>(vector<T, N> &, vector<T, N> &) noexcept;                                             \
	friend constexpr auto operator<=><>(vector<T, N>, vector<T, N>) noexcept;                                          \
	friend constexpr bool operator==<>(vector<T, N>, vector<T, N>) noexcept;                                           \
	friend constexpr vector<T, N> operator+<>(vector<T, N>, vector<T, N>) noexcept;                                    \
	friend constexpr vector<T, N> &operator+=<>(vector<T, N> &, vector<T, N>) noexcept;                                \
	friend constexpr vector<T, N> operator-<>(vector<T, N>, vector<T, N>) noexcept;                                    \
	friend constexpr vector<T, N> &operator-=<>(vector<T, N> &, vector<T, N>) noexcept;                                \
	friend constexpr vector<T, N> operator*<>(vector<T, N>, T) noexcept;                                               \
	friend constexpr vector<T, N> &operator*=<>(vector<T, N> &, T) noexcept;                                           \
	friend constexpr vector<T, N> operator/<>(vector<T, N>, T) noexcept;                                               \
	friend constexpr vector<T, N> &operator/=<>(vector<T, N> &, T) noexcept;                                           \
	friend constexpr vector<T, N> max<>(vector<T, N>, vector<T, N>) noexcept;                                          \
	friend constexpr vector<T, N> min<>(vector<T, N>, vector<T, N>) noexcept;                                          \
	friend constexpr T dot<>(vector<T, N>, vector<T, N>) noexcept;                                                     \
	friend constexpr vector<T, N> sqrt<>(vector<T, N>) noexcept;                                                       \
	friend constexpr vector<T, N> rsqrt<>(vector<T, N>) noexcept;                                                      \
	friend constexpr T magn<>(vector<T, N>) noexcept;                                                                  \
	friend constexpr vector<T, N> norm<>(vector<T, N>) noexcept;                                                       \
                                                                                                                       \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr vector<U, M> operator+(vector<U, M>) noexcept requires std::is_signed_v<U>;                       \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr vector<U, M> operator-(vector<U, M>) noexcept requires std::is_signed_v<U>;                       \
	template<typename U, std::size_t M>                                                                                \
	friend constexpr vector<U, M> abs(vector<U, M>) noexcept requires std::is_signed_v<U>;                             \
                                                                                                                       \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr vector<U, M> &operator&=(vector<U, M> &, vector<U, M>) noexcept;                                  \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr vector<U, M> operator&(vector<U, M>, vector<U, M>) noexcept;                                      \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr vector<U, M> &operator|=(vector<U, M> &, vector<U, M>) noexcept;                                  \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr vector<U, M> operator|(vector<U, M>, vector<U, M>) noexcept;                                      \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr vector<U, M> operator^(vector<U, M>, vector<U, M>) noexcept;                                      \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr vector<U, M> &operator^=(vector<U, M> &, vector<U, M>) noexcept;                                  \
	template<std::integral U, std::size_t M>                                                                           \
	friend constexpr vector<U, M> operator~(vector<U, M>) noexcept;                                                    \
                                                                                                                       \
public:                                                                                                                \
	typedef T value_type;                                                                                              \
	constexpr vector() noexcept = default;                                                                             \
	constexpr explicit vector(const value_type(&vals)[N]) noexcept : data(vals) {}                                     \
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
	[[nodiscard]] constexpr operator vector<value_type, M>() const noexcept                                            \
	{                                                                                                                  \
		return vector_cast<M>(*this);                                                                                  \
	}                                                                                                                  \
	constexpr void swap(vector &other) noexcept { data.swap(other.data); }

namespace sek::math
{
	template<std::size_t N, std::size_t M, typename T>
	[[nodiscard]] constexpr vector<T, N> vector_cast(const vector<T, M> &) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr sek::hash_t hash(const vector<T, N> &) noexcept;
	template<typename T, std::size_t N>
	constexpr void swap(vector<T, N> &a, vector<T, N> &) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr auto operator<=>(vector<T, N>, vector<T, N>) noexcept;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr bool operator==(vector<T, N>, vector<T, N>) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator+(vector<T, N>, vector<T, N>) noexcept;
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator+=(vector<T, N> &, vector<T, N>) noexcept;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator-(vector<T, N>, vector<T, N>) noexcept;
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator-=(vector<T, N> &, vector<T, N>) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator+(vector<T, N>) noexcept requires std::is_signed_v<T>;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator-(vector<T, N>) noexcept requires std::is_signed_v<T>;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator*(vector<T, N>, T) noexcept;
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator*=(vector<T, N> &, T) noexcept;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator/(vector<T, N>, T) noexcept;
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator/=(vector<T, N> &, T) noexcept;

	template<std::integral T, std::size_t N>
	constexpr vector<T, N> &operator&=(vector<T, N> &, vector<T, N>) noexcept;
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator&(vector<T, N>, vector<T, N>) noexcept;
	template<std::integral T, std::size_t N>
	constexpr vector<T, N> &operator|=(vector<T, N> &, vector<T, N>) noexcept;
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator|(vector<T, N>, vector<T, N>) noexcept;
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator^(vector<T, N>, vector<T, N>) noexcept;
	template<std::integral T, std::size_t N>
	constexpr vector<T, N> &operator^=(vector<T, N> &, vector<T, N>) noexcept;
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator~(vector<T, N>) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> abs(vector<T, N>) noexcept requires std::is_signed_v<T>;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> max(vector<T, N>, vector<T, N>) noexcept;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> min(vector<T, N>, vector<T, N>) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr T dot(vector<T, N>, vector<T, N>) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> sqrt(vector<T, N>) noexcept;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> rsqrt(vector<T, N>) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr T magn(vector<T, N>) noexcept;
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> norm(vector<T, N>) noexcept;

	template<typename T, std::size_t N>
	requires(N != 0 && arithmetic<T>) union vector
	{
		SEK_MATH_VECTOR_COMMON(T, N)

	private:
		template<typename... U>
		constexpr static bool compatible_type_seq = std::conjunction_v<std::is_convertible<U, T>...>;

	public:
		template<arithmetic... U>
		constexpr explicit vector(U... data) noexcept requires compatible_type_seq<U...> : data({data...})
		{
		}
	};

	template<arithmetic T>
	union vector<T, 2>
	{
		SEK_MATH_VECTOR_COMMON(T, 2)

		constexpr vector(T x, T y) noexcept : data({x, y}) {}
		constexpr explicit vector(T x) noexcept : vector(x, T{}) {}

#ifndef SEK_NO_ANONYMOUS_STRUCT
		struct
		{
			T x;
			T y;
		};
#endif
	};

	template<arithmetic T>
	union vector<T, 3>
	{
		SEK_MATH_VECTOR_COMMON(T, 3)

		constexpr vector(T x, T y, T z) noexcept : data({x, y, z}) {}
		constexpr vector(T x, T y) noexcept : vector(x, y, T{}) {}
		constexpr explicit vector(T x) noexcept : vector(x, T{}, T{}) {}

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
	union vector<T, 4>
	{
		SEK_MATH_VECTOR_COMMON(T, 4)

		constexpr vector(T x, T y, T z, T w) noexcept : data({x, y, z, w}) {}
		constexpr vector(T x, T y, T z) noexcept : vector(x, y, z, T{}) {}
		constexpr vector(T x, T y) noexcept : vector(x, y, T{}, T{}) {}
		constexpr explicit vector(T x) noexcept : vector(x, T{}, T{}, T{}) {}

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
	[[nodiscard]] constexpr vector<T, N> vector_cast(const vector<T, M> &src) noexcept
	{
		constexpr auto make_vector = []<std::size_t... Is>(std::index_sequence<Is...>, const vector<T, M> &src)
		{
			constexpr auto extract = []<std::size_t I>(const vector<T, M> &src)
			{
				if constexpr (I > M)
					return T{};
				else
					return src[I];
			};

			return vector<T, N>{{extract<Is>(src)...}};
		};
		return make_vector(std::make_index_sequence<N>(), src);
	}

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr sek::hash_t hash(const vector<T, N> &v) noexcept
	{
		return v.data.hash();
	}
	template<typename T, std::size_t N>
	constexpr void swap(vector<T, N> &a, vector<T, N> &b) noexcept
	{
		a.swap(b);
	}

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr auto operator<=>(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		return lhs.data <=> rhs.data;
	}
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr bool operator==(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		return lhs.data == rhs.data;
	}

	/** Returns a vector which is the result of addition of two vectors. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator+(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		vector<T, N> result;
		detail::vector_add(result.data, lhs.data, rhs.data);
		return result;
	}
	/** Adds a vector to a vector. */
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator+=(vector<T, N> &lhs, vector<T, N> rhs) noexcept
	{
		detail::vector_add(lhs.data, lhs.data, rhs.data);
		return lhs;
	}
	/** Returns a vector which is the result of subtraction of two vectors. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator-(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		vector<T, N> result;
		detail::vector_sub(result.data, lhs.data, rhs.data);
		return result;
	}
	/** Subtracts a vector from a vector. */
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator-=(vector<T, N> &lhs, vector<T, N> rhs) noexcept
	{
		detail::vector_sub(lhs.data, lhs.data, rhs.data);
		return lhs;
	}

	/** Returns a copy of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator+(vector<T, N> v) noexcept requires std::is_signed_v<T>
	{
		return v;
	}
	/** Returns a negated copy of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator-(vector<T, N> v) noexcept requires std::is_signed_v<T>
	{
		vector<T, N> result;
		detail::vector_mul(result.data, v.data);
		return result;
	}

	/** Returns a copy of the vector multiplied by a scalar. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator*(vector<T, N> lhs, T rhs) noexcept
	{
		vector<T, N> result;
		detail::vector_mul(result.data, lhs.data, rhs);
		return result;
	}
	/** Multiplies vector by a scalar. */
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator*=(vector<T, N> &lhs, T rhs) noexcept
	{
		detail::vector_mul(lhs.data, lhs.data, rhs);
		return lhs;
	}
	/** Returns a copy of the vector divided by a scalar. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator/(vector<T, N> lhs, T rhs) noexcept
	{
		vector<T, N> result;
		detail::vector_div(result.data, lhs.data, rhs);
		return result;
	}
	/** Divides vector by a scalar. */
	template<typename T, std::size_t N>
	constexpr vector<T, N> &operator/=(vector<T, N> &lhs, T rhs) noexcept
	{
		detail::vector_div(lhs.data, lhs.data, rhs);
		return lhs;
	}

	/** Preforms a bitwise AND on two vectors. */
	template<std::integral T, std::size_t N>
	constexpr vector<T, N> &operator&=(vector<T, N> &lhs, vector<T, N> rhs) noexcept
	{
		detail::vector_and(lhs.data, lhs.data, rhs.data);
		return lhs;
	}
	/** Returns a vector which is the result of bitwise AND of two vectors. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator&(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		vector<T, N> result;
		detail::vector_and(result.data, lhs.data, rhs.data);
		return result;
	}
	/** Preforms a bitwise OR on two vectors. */
	template<std::integral T, std::size_t N>
	constexpr vector<T, N> &operator|=(vector<T, N> &lhs, vector<T, N> rhs) noexcept
	{
		detail::vector_or(lhs.data, lhs.data, rhs.data);
		return lhs;
	}
	/** Returns a vector which is the result of bitwise OR of two vectors. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator|(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		vector<T, N> result;
		detail::vector_or(result.data, lhs.data, rhs.data);
		return result;
	}
	/** Returns a vector which is the result of bitwise XOR of two vectors. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator^(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		vector<T, N> result;
		detail::vector_xor(result.data, lhs.data, rhs.data);
		return result;
	}
	/** Preforms a bitwise XOR on two vectors. */
	template<std::integral T, std::size_t N>
	constexpr vector<T, N> &operator^=(vector<T, N> &lhs, vector<T, N> rhs) noexcept
	{
		detail::vector_xor(lhs.data, lhs.data, rhs.data);
		return lhs;
	}
	/** Returns a bitwise inverted copy of a vector. */
	template<std::integral T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> operator~(vector<T, N> v) noexcept
	{
		vector<T, N> result;
		detail::vector_inv(result.data, v.data);
		return result;
	}

	/** Calculates absolute value of a vector.
	 * @example abs({-1, 2, 0}) -> {1, 2, 0} */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> abs(vector<T, N> v) noexcept requires std::is_signed_v<T>
	{
		vector<T, N> result;
		detail::vector_abs(result.data, v.data);
		return result;
	}
	/** Returns a vector consisting of maximum data of a and b.
	 * @example max({0, 1, 3}, {-1, 2, 2}) -> {0, 2, 3} */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> max(vector<T, N> a, vector<T, N> b) noexcept
	{
		vector<T, N> result;
		detail::vector_max(result.data, a.data, b.data);
		return result;
	}
	/** Returns a vector consisting of minimum data of a and b.
	 * @example min({0, 1, 3}, {-1, 2, 2}) -> {-1, 1, 2} */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> min(vector<T, N> a, vector<T, N> b) noexcept
	{
		vector<T, N> result;
		detail::vector_min(result.data, a.data, b.data);
		return result;
	}

	/** Calculates dot product of two vectors. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr T dot(vector<T, N> lhs, vector<T, N> rhs) noexcept
	{
		return detail::vector_dot(lhs.data, rhs.data);
	}

	/** Calculates square root of a vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> sqrt(vector<T, N> v) noexcept
	{
		vector<T, N> result;
		detail::vector_sqrt(result.data, v.data);
		return result;
	}
	/** Calculates reciprocal square root of a vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> rsqrt(vector<T, N> v) noexcept
	{
		vector<T, N> result;
		detail::vector_rsqrt(result.data, v.data);
		return result;
	}

	/** Returns a length of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr T magn(vector<T, N> v) noexcept
	{
		/* Magnitude of a vector A=XYZ is sqrt(X*X + Y*Y + Z*Z) = sqrt(dot(A, A)). */
		return std::sqrt(dot(v, v));
	}
	/** Returns a normalized copy of the vector. */
	template<typename T, std::size_t N>
	[[nodiscard]] constexpr vector<T, N> norm(vector<T, N> v) noexcept
	{
		vector<T, N> result = {};
		detail::vector_norm(result.data, v.data);
		return result;
	}
}	 // namespace sek::math
