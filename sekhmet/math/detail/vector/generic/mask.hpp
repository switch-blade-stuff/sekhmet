/*
 * Created by switchblade on 24/06/22
 */

#pragma once

#include "../../util.hpp"
#include "../storage.hpp"

namespace sek::math::detail
{
	inline namespace generic
	{
		template<std::size_t I, std::size_t N, typename F, typename... Args>
		constexpr void mask_unwrap(F &&f, Args &&...args)
		{
			if constexpr (I != N)
			{
				f(I, std::forward<Args>(args)...);
				vector_unwrap<I + 1, N>(std::forward<F>(f), std::forward<Args>(args)...);
			}
		}
		template<std::size_t N, typename F, typename... Args>
		constexpr void mask_unwrap(F &&f, Args &&...args)
		{
			mask_unwrap<0, N>(std::forward<F>(f), std::forward<Args>(args)...);
		}

		template<std::size_t J, typename T, std::size_t N, std::size_t M, storage_policy P, std::size_t I, std::size_t... Is>
		constexpr void shuffle_unwrap(mask_data<T, N, P> &out, const mask_data<T, M, P> &l, std::index_sequence<I, Is...>) noexcept
		{
			out[J] = l[I];
			if constexpr (sizeof...(Is) != 0) shuffle_unwrap<J + 1>(out, l, std::index_sequence<Is...>{});
		}
		template<typename T, std::size_t N, std::size_t M, storage_policy P, std::size_t... Is>
		constexpr void mask_shuffle(mask_data<T, N, P> &out, const mask_data<T, M, P> &l, std::index_sequence<Is...> s) noexcept
		{
			shuffle_unwrap<0>(out, l, s);
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void mask_eq(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		{
			mask_unwrap<N>([&](auto i) { out[i] = l[i] == r[i]; });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void mask_ne(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		{
			mask_unwrap<N>([&](auto i) { out[i] = l[i] != r[i]; });
		}

		template<typename T, std::size_t N, storage_policy P>
		constexpr void mask_and(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		{
			mask_unwrap<N>([&](auto i) { out[i] = static_cast<bool>(l[i] && r[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void mask_or(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		{
			mask_unwrap<N>([&](auto i) { out[i] = static_cast<bool>(l[i] || r[i]); });
		}
		template<typename T, std::size_t N, storage_policy P>
		constexpr void mask_neg(mask_data<T, N, P> &out, const mask_data<T, N, P> &l) noexcept
		{
			mask_unwrap<N>([&](auto i) { out[i] = static_cast<bool>(!l[i]); });
		}
	}	 // namespace generic
}	 // namespace sek::math::detail