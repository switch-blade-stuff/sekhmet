/*
 * Created by switchblade on 24/06/22
 */

#pragma once

#include "shuffle.hpp"
#include "storage.hpp"
#include "util.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/mask_double.hpp"
#include "x86/mask_float.hpp"
#include "x86/mask_int32.hpp"
#include "x86/mask_int64.hpp"
#else
#warning "SMID vector operations are not implemented for this CPU"
#define SEK_NO_SIMD
#endif
#endif

#include "generic/mask.hpp"

#define SEK_DETAIL_VECTOR_MASK_COMMON(T, N, P)                                                                                    \
private:                                                                                                                          \
	using data_t = detail::mask_data<T, N, P>;                                                                                    \
	data_t m_data = {};                                                                                                           \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr sek::hash_t hash(const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                                            \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr void swap(vec_mask<basic_vec<U, M, Sp>> &, vec_mask<basic_vec<U, M, Sp>> &) noexcept;                        \
                                                                                                                                  \
	template<std::size_t... I, typename U, std::size_t M, storage_policy Sp>                                                      \
	friend constexpr vec_mask<basic_vec<U, sizeof...(I), Sp>> shuffle(const vec_mask<basic_vec<U, M, Sp>> &) noexcept;            \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator&&(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator||(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator!(const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                     \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> interleave(                                                                              \
		const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                \
                                                                                                                                  \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> &operator&=(basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;      \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> operator&(const basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;  \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> &operator|=(basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;      \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> operator|(const basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;  \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> operator^(const basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;  \
	template<std::integral U, std::size_t M, storage_policy Sp>                                                                   \
	friend constexpr basic_vec<U, M, Sp> &operator^=(basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;      \
                                                                                                                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator<(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator<=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator>(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;  \
	template<typename U, std::size_t M, storage_policy Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator>=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
                                                                                                                                  \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_nan(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_inf(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_fin(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_neg(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<std::floating_point U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> is_norm(const basic_vec<U, M, Sp> &) noexcept;                                 \
                                                                                                                                  \
public:                                                                                                                           \
	typedef bool value_type;                                                                                                      \
                                                                                                                                  \
	constexpr static auto extent = N;                                                                                             \
	constexpr static auto policy = P;                                                                                             \
                                                                                                                                  \
	constexpr vec_mask() noexcept = default;                                                                                      \
                                                                                                                                  \
	template<std::convertible_to<T> U, std::size_t M, storage_policy Sp>                                                          \
	constexpr explicit vec_mask(const vec_mask<basic_vec<U, M, Sp>> &other) noexcept                                              \
		requires(!std::same_as<T, U> || M != N || Sp != P)                                                                        \
	{                                                                                                                             \
		for (std::size_t i = 0; i < min(M, N); ++i) m_data[i] = other.m_data[i];                                                  \
	}                                                                                                                             \
                                                                                                                                  \
	template<std::size_t M>                                                                                                       \
	constexpr explicit vec_mask(const value_type(&vals)[N]) noexcept : m_data(vals)                                               \
	{                                                                                                                             \
	}                                                                                                                             \
                                                                                                                                  \
	/** Returns the N-th element of the mask. */                                                                                  \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) noexcept                                                     \
	{                                                                                                                             \
		return m_data[i];                                                                                                         \
	} /** @copydoc operator[] */                                                                                                  \
	[[nodiscard]] constexpr decltype(auto) operator[](std::size_t i) const noexcept                                               \
	{                                                                                                                             \
		return m_data[i];                                                                                                         \
	}                                                                                                                             \
	constexpr void swap(vec_mask &other) noexcept                                                                                 \
	{                                                                                                                             \
		m_data.swap(other.m_data);                                                                                                \
	}

#define SEK_DETAIL_M_TYPE(Extent) vec_mask<basic_vec<T, Extent, Policy>>
#define SEK_DETAIL_M_SHUFFLE(...) (shuffle<__VA_ARGS__>(*this))
#define SEK_DETAIL_M_SHUFFLE_2(x, y) SEK_DETAIL_SHUFFLE_2(SEK_DETAIL_M_SHUFFLE, SEK_DETAIL_M_TYPE, x, y)
#define SEK_DETAIL_M_SHUFFLE_3(x, y, z) SEK_DETAIL_SHUFFLE_3(SEK_DETAIL_M_SHUFFLE, SEK_DETAIL_M_TYPE, x, y, z)
#define SEK_DETAIL_M_SHUFFLE_4(x, y, z, w) SEK_DETAIL_SHUFFLE_4(SEK_DETAIL_M_SHUFFLE, SEK_DETAIL_M_TYPE, x, y, z, w)
#define SEK_VECTOR_MASK_GENERATE_SHUFFLE(x, ...)                                                                       \
	SEK_GET_MACRO_3(__VA_ARGS__, SEK_DETAIL_M_SHUFFLE_4, SEK_DETAIL_M_SHUFFLE_3, SEK_DETAIL_M_SHUFFLE_2)               \
	(x, __VA_ARGS__)

namespace sek::math
{
	template<typename T, std::size_t N, storage_policy Policy>
	class basic_vec;
	template<typename...>
	class vec_mask;

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

	/** Produces a new vector mask which is the result of shuffling elements of another mask.
	 * @tparam I Indices of elements of the source mask in the order they should be shuffled to the destination mask.
	 * @return Result mask who's elements are specified by `I`. */
	template<std::size_t... I, typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, sizeof...(I), Sp>> shuffle(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		using Idx = std::index_sequence<I...>;
		if constexpr (std::is_same_v<Idx, std::make_index_sequence<M>>)
			return m;
		else
		{
			vec_mask<basic_vec<U, sizeof...(I), Sp>> result;
			if (std::is_constant_evaluated())
				detail::generic::mask_shuffle(result.m_data, m.m_data, Idx{});
			else
				detail::mask_shuffle(result.m_data, m.m_data, Idx{});
			return result;
		}
	}

	/** Gets the Ith element of the vector mask. */
	template<std::size_t I, typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr decltype(auto) get(vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		return m[I];
	}
	/** @copydoc get */
	template<std::size_t I, typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr decltype(auto) get(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		return m[I];
	}

	namespace detail
	{
		template<std::size_t... Is, typename U, std::size_t M, storage_policy Sp, typename F>
		constexpr void vectorize_impl(std::index_sequence<Is...>, const vec_mask<basic_vec<U, M, Sp>> &m, F &&f)
		{
			(f(get<Is>(m)), ...);
		}
		template<std::size_t... Is, typename U, std::size_t M, storage_policy Sp, typename F>
		constexpr void vectorize_impl(std::index_sequence<Is...>, vec_mask<basic_vec<U, M, Sp>> &m, F &&f)
		{
			(f(get<Is>(m)), ...);
		}
	}	 // namespace detail

	/** Applies a functor to every element of the vector mask. */
	template<typename U, std::size_t M, storage_policy Sp, typename F>
	constexpr void vectorize(const vec_mask<basic_vec<U, M, Sp>> &m, F &&f)
	{
		detail::vectorize_impl(std::make_index_sequence<M>{}, m, std::forward<F>(f));
	}
	/** @copydoc vectorize */
	template<typename U, std::size_t M, storage_policy Sp, typename F>
	constexpr void vectorize(vec_mask<basic_vec<U, M, Sp>> &m, F &&f)
	{
		detail::vectorize_impl(std::make_index_sequence<M>{}, m, std::forward<F>(f));
	}

	/** Checks if all components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr bool all(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		bool result = true;
		vectorize(m, [&result](auto value) noexcept { result = result && static_cast<bool>(value); });
		return result;
	}
	/** Checks if any components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr bool any(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		bool result = false;
		vectorize(m, [&result](auto value) noexcept { result = result || static_cast<bool>(value); });
		return result;
	}
	/** Checks if no components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr bool none(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		return !any(m);
	}

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_eq(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_eq(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_ne(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_ne(result.m_data, l.m_data, r.m_data);
		return result;
	}

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator&&(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_and(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_and(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator||(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_or(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_or(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!(const vec_mask<basic_vec<U, M, Sp>> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_neg(result.m_data, v.m_data);
		else
			detail::mask_neg(result.m_data, v.m_data);
		return result;
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
