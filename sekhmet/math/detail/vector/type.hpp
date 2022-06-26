/*
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include "shuffle.hpp"
#include "storage.hpp"

/* vector & mask function declarations. */
#include "func/arithmetic.hpp"
#include "func/bitwise.hpp"
#include "func/category.hpp"
#include "func/geometric.hpp"
#include "func/relational.hpp"
#include "func/trigonometric.hpp"
#include "func/util.hpp"

#define SEK_DETAIL_VECTOR_MASK_COMMON(T, N, P)                                                                         \
private:                                                                                                               \
	using data_t = detail::mask_data<T, N, P>;                                                                         \
	data_t m_data = {};                                                                                                \
                                                                                                                       \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr sek::hash_t hash(const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                                 \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr void swap(vec_mask<basic_vec<U, M, Sp>> &, vec_mask<basic_vec<U, M, Sp>> &) noexcept;             \
                                                                                                                       \
	SEK_DETAIL_FRIEND_UTILITY                                                                                          \
	SEK_DETAIL_FRIEND_RELATIONAL                                                                                       \
	SEK_DETAIL_FRIEND_CATEGORY                                                                                         \
                                                                                                                       \
public:                                                                                                                \
	typedef bool value_type;                                                                                           \
                                                                                                                       \
	constexpr static auto extent = N;                                                                                  \
	constexpr static auto policy = P;                                                                                  \
                                                                                                                       \
	constexpr vec_mask() noexcept = default;                                                                           \
                                                                                                                       \
	template<std::convertible_to<T> U, std::size_t M, storage_policy Sp>                                               \
	constexpr explicit vec_mask(const vec_mask<basic_vec<U, M, Sp>> &other) noexcept                                   \
		requires(!std::same_as<T, U> || M != N || Sp != P)                                                             \
	{                                                                                                                  \
		for (std::size_t i = 0; i < min(M, N); ++i) m_data[i] = other.m_data[i];                                       \
	}                                                                                                                  \
                                                                                                                       \
	template<std::size_t M>                                                                                            \
	constexpr explicit vec_mask(const value_type(&vals)[N]) noexcept : m_data(vals)                                    \
	{                                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	/** Returns the N-th element of the mask. */                                                                       \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) noexcept                                          \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	} /** @copydoc operator[] */                                                                                       \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) const noexcept                                    \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	}                                                                                                                  \
	constexpr void swap(vec_mask &other) noexcept                                                                      \
	{                                                                                                                  \
		m_data.swap(other.m_data);                                                                                     \
	}

#define SEK_DETAIL_M_TYPE(Extent) vec_mask<basic_vec<T, Extent, Policy>>
#define SEK_DETAIL_M_SHUFFLE_2(x, y) SEK_DETAIL_SHUFFLE_2(SEK_DETAIL_M_TYPE, x, y)
#define SEK_DETAIL_M_SHUFFLE_3(x, y, z) SEK_DETAIL_SHUFFLE_3(SEK_DETAIL_M_TYPE, x, y, z)
#define SEK_DETAIL_M_SHUFFLE_4(x, y, z, w) SEK_DETAIL_SHUFFLE_4(SEK_DETAIL_M_TYPE, x, y, z, w)
#define SEK_VECTOR_MASK_GENERATE_SHUFFLE(x, ...)                                                                       \
	SEK_GET_MACRO_3(__VA_ARGS__, SEK_DETAIL_M_SHUFFLE_4, SEK_DETAIL_M_SHUFFLE_3, SEK_DETAIL_M_SHUFFLE_2)               \
	(x, __VA_ARGS__)

#define SEK_DETAIL_VECTOR_COMMON(T, N, P)                                                                              \
private:                                                                                                               \
	using data_t = detail::vector_data<T, N, P>;                                                                       \
	using mask_t = vec_mask<basic_vec>;                                                                                \
	data_t m_data = {};                                                                                                \
                                                                                                                       \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr sek::hash_t hash(const basic_vec<U, M, Sp> &) noexcept;                                           \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr void swap(basic_vec<U, M, Sp> &, basic_vec<U, M, Sp> &) noexcept;                                 \
                                                                                                                       \
	SEK_DETAIL_FRIEND_UTILITY                                                                                          \
	SEK_DETAIL_FRIEND_RELATIONAL                                                                                       \
	SEK_DETAIL_FRIEND_CATEGORY                                                                                         \
	SEK_DETAIL_FRIEND_ARITHMETIC                                                                                       \
	SEK_DETAIL_FRIEND_TRIGONOMETRIC                                                                                    \
	SEK_DETAIL_FRIEND_BITWISE                                                                                          \
	SEK_DETAIL_FRIEND_GEOMETRIC                                                                                        \
                                                                                                                       \
public:                                                                                                                \
	typedef T value_type;                                                                                              \
	typedef mask_t mask_type;                                                                                          \
                                                                                                                       \
	constexpr static auto extent = N;                                                                                  \
	constexpr static auto policy = P;                                                                                  \
                                                                                                                       \
	constexpr basic_vec() noexcept = default;                                                                          \
                                                                                                                       \
	template<std::convertible_to<T> U, std::size_t M, storage_policy OtherPolicy>                                      \
	constexpr explicit basic_vec(const basic_vec<U, M, OtherPolicy> &other) noexcept                                   \
		requires(!std::same_as<T, U> || M != N || OtherPolicy != P)                                                    \
	{                                                                                                                  \
		for (std::size_t i = 0; i < min(M, N); ++i) m_data[i] = other.m_data[i];                                       \
	}                                                                                                                  \
                                                                                                                       \
	template<std::size_t M>                                                                                            \
	constexpr explicit basic_vec(const value_type(&vals)[N]) noexcept : m_data(vals)                                   \
	{                                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	/** Returns the N-th element of the vector. */                                                                     \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) noexcept                                          \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) const noexcept                                    \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	}                                                                                                                  \
	constexpr void swap(basic_vec &other) noexcept                                                                     \
	{                                                                                                                  \
		m_data.swap(other.m_data);                                                                                     \
	}

#define SEK_DETAIL_V_TYPE(Extent) basic_vec<T, Extent, Policy>
#define SEK_DETAIL_V_SHUFFLE_2(x, y) SEK_DETAIL_SHUFFLE_2(SEK_DETAIL_V_TYPE, x, y)
#define SEK_DETAIL_V_SHUFFLE_3(x, y, z) SEK_DETAIL_SHUFFLE_3(SEK_DETAIL_V_TYPE, x, y, z)
#define SEK_DETAIL_V_SHUFFLE_4(x, y, z, w) SEK_DETAIL_SHUFFLE_4(SEK_DETAIL_V_TYPE, x, y, z, w)
#define SEK_VECTOR_GENERATE_SHUFFLE(x, ...)                                                                            \
	SEK_GET_MACRO_3(__VA_ARGS__, SEK_DETAIL_V_SHUFFLE_4, SEK_DETAIL_V_SHUFFLE_3, SEK_DETAIL_V_SHUFFLE_2)               \
	(x, __VA_ARGS__)

namespace sek::math
{
	/** @brief Structure representing a mathematical vector.
	 * @tparam T Type of values stored in the vector.
	 * @tparam N Amount of values the vector holds.
	 * @tparam Policy Policy used for storage & optimization. */
	template<typename T, std::size_t N, storage_policy Policy = storage_policy::OPTIMAL>
	class basic_vec;
	/** @brief Structure used to mask off elements of a vector. */
	template<typename T, std::size_t N, storage_policy P>
	class vec_mask<basic_vec<T, N, P>>;

	template<typename T, storage_policy Policy>
	class vec_mask<basic_vec<T, 2, Policy>>
	{
		template<typename...>
		friend class vec_mask;

	public:
		SEK_DETAIL_VECTOR_MASK_COMMON(T, 2, Policy)

	public:
		constexpr vec_mask(bool x, bool y) noexcept : m_data(x, y) {}
		constexpr explicit vec_mask(bool x) noexcept : vec_mask(x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }

		SEK_VECTOR_MASK_GENERATE_SHUFFLE(x, y)
	};
	template<typename T, storage_policy Policy>
	class vec_mask<basic_vec<T, 3, Policy>>
	{
		template<typename...>
		friend class vec_mask;

	public:
		SEK_DETAIL_VECTOR_MASK_COMMON(T, 3, Policy)

	public:
		constexpr vec_mask(bool x, bool y, bool z) noexcept : m_data(x, y, z) {}
		constexpr vec_mask(bool x, bool y) noexcept : vec_mask(x, y, y) {}
		constexpr explicit vec_mask(bool x) noexcept : vec_mask(x, x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data[2]; }

		SEK_VECTOR_MASK_GENERATE_SHUFFLE(x, y, z)
	};
	template<typename T, storage_policy Policy>
	class vec_mask<basic_vec<T, 4, Policy>>
	{
		template<typename...>
		friend class vec_mask;

	public:
		SEK_DETAIL_VECTOR_MASK_COMMON(T, 4, Policy)

	public:
		constexpr vec_mask(bool x, bool y, bool z, bool w) noexcept : m_data(x, y, z, w) {}
		constexpr vec_mask(bool x, bool y, bool z) noexcept : vec_mask(x, y, z, z) {}
		constexpr vec_mask(bool x, bool y) noexcept : vec_mask(x, y, y, y) {}
		constexpr explicit vec_mask(bool x) noexcept : vec_mask(x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) w() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr decltype(auto) w() const noexcept { return m_data[3]; }

		SEK_VECTOR_MASK_GENERATE_SHUFFLE(x, y, z, w)
	};

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr sek::hash_t hash(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		hash_t result = 0;
		for (std::size_t i = 0; i < M; ++i) hash_combine(result, m[i]);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr void swap(vec_mask<basic_vec<U, M, Sp>> &a, vec_mask<basic_vec<U, M, Sp>> &b) noexcept
	{
		a.swap(b);
	}

	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 2, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_DETAIL_VECTOR_COMMON(T, 2, Policy)

	public:
		constexpr basic_vec(T x, T y) noexcept : m_data(x, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y)
	};
	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 3, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_DETAIL_VECTOR_COMMON(T, 3, Policy)

	public:
		constexpr basic_vec(T x, T y, T z) noexcept : m_data(x, y, z) {}
		constexpr basic_vec(T x, T y) noexcept : basic_vec(x, y, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z)

		[[nodiscard]] constexpr decltype(auto) r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) b() const noexcept { return m_data[2]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b)
	};
	template<arithmetic T, storage_policy Policy>
	class basic_vec<T, 4, Policy>
	{
		template<typename U, std::size_t M, storage_policy P>
		friend class basic_vec;

	public:
		SEK_DETAIL_VECTOR_COMMON(T, 4, Policy)

	public:
		constexpr basic_vec(T x, T y, T z, T w) noexcept : m_data(x, y, z, w) {}
		constexpr basic_vec(T x, T y, T z) noexcept : basic_vec(x, y, z, z) {}
		constexpr basic_vec(T x, T y) noexcept : basic_vec(x, y, y, y) {}
		constexpr explicit basic_vec(T x) noexcept : basic_vec(x, x, x, x) {}

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) w() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr decltype(auto) w() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(x, y, z, w)

		[[nodiscard]] constexpr decltype(auto) r() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) r() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) g() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) g() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) b() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) b() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) a() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr decltype(auto) a() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(r, g, b, a)

		[[nodiscard]] constexpr decltype(auto) s() noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) s() const noexcept { return m_data[0]; }
		[[nodiscard]] constexpr decltype(auto) t() noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) t() const noexcept { return m_data[1]; }
		[[nodiscard]] constexpr decltype(auto) p() noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) p() const noexcept { return m_data[2]; }
		[[nodiscard]] constexpr decltype(auto) q() noexcept { return m_data[3]; }
		[[nodiscard]] constexpr decltype(auto) q() const noexcept { return m_data[3]; }

		SEK_VECTOR_GENERATE_SHUFFLE(s, t, p, q)
	};

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr sek::hash_t hash(const basic_vec<U, M, Sp> &v) noexcept
	{
		hash_t result = 0;
		for (std::size_t i = 0; i < M; ++i) hash_combine(result, v[i]);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	constexpr void swap(basic_vec<U, M, Sp> &a, basic_vec<U, M, Sp> &b) noexcept
	{
		a.swap(b);
	}
}	 // namespace sek::math

template<typename U, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_size<sek::math::vec_mask<sek::math::basic_vec<U, M, Sp>>> : std::integral_constant<std::size_t, M>
{
};
template<std::size_t I, typename U, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_element<I, sek::math::vec_mask<sek::math::basic_vec<U, M, Sp>>>
{
	using type = bool;
};

template<typename U, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_size<sek::math::basic_vec<U, M, Sp>> : std::integral_constant<std::size_t, M>
{
};
template<std::size_t I, typename U, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_element<I, sek::math::basic_vec<U, M, Sp>>
{
	using type = U;
};
