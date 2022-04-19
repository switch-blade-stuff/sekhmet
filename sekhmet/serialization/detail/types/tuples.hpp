//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include <concepts>
#include <tuple>
#include <utility>

#include "../manipulators.hpp"

namespace sek::serialization
{
	namespace detail
	{
		// clang-format off
		template<typename T, std::size_t N>
		concept has_tuple_element = requires(T t)
		{
			typename std::tuple_element_t<N, std::remove_const_t<T>>;
			{ get<N>(t) } -> std::convertible_to<const std::tuple_element_t<N, T> &>;
		};
		template<typename T, std::size_t... Is>
		constexpr auto has_tuple_elements(std::index_sequence<Is...>)
		{
			return (has_tuple_element<T, Is> && ...);
		}
		template<typename T>
		concept tuple_like = !std::is_reference_v<T> && requires(T t)
		{
			typename std::tuple_size<T>::type;
			std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
		} && has_tuple_elements<T>(std::make_index_sequence<std::tuple_size_v<T>>());
		template<typename T>
		concept pair_like = requires(T &p)
		{
			p.first;
			p.second;
		};
		// clang-format on

		template<detail::tuple_like T, typename A, std::size_t I, std::size_t... Is>
		void tuple_serialize_unwrap(const T &t, A &a, std::index_sequence<I, Is...>)
		{
			using std::get;
			a << get<I>(t);
			if constexpr (sizeof...(Is) != 0) tuple_serialize_unwrap(t, a, std::index_sequence<Is...>{});
		}
		template<detail::tuple_like T, typename A, std::size_t I, std::size_t... Is>
		void tuple_deserialize_unwrap(T &t, A &a, std::index_sequence<I, Is...>)
		{
			using std::get;
			a >> get<I>(t);
			if constexpr (sizeof...(Is) != 0) tuple_deserialize_unwrap(t, a, std::index_sequence<Is...>{});
		}
	}	 // namespace detail

	template<detail::tuple_like T, typename A>
	void serialize(const T &tuple, A &archive) requires(!std::ranges::forward_range<T>)
	{
		if constexpr (std::tuple_size_v<T> != 0)
		{
			archive << array_mode() << container_size(std::tuple_size_v<T>);
			detail::tuple_serialize_unwrap(tuple, archive, std::make_index_sequence<std::tuple_size_v<T>>{});
		}
	}
	template<detail::tuple_like T, typename A>
	void deserialize(T &tuple, A &archive) requires(!std::ranges::forward_range<T>)
	{
		if constexpr (std::tuple_size_v<T> != 0)
			detail::tuple_deserialize_unwrap(tuple, archive, std::make_index_sequence<std::tuple_size_v<T>>{});
	}

	template<detail::pair_like T, typename A>
	void serialize(const T &pair, A &archive) requires(!detail::tuple_like<T>)
	{
		/* Treat pairs as dynamic-size arrays, since in most cases using a fixed size will have more overhead (need to store a size). */
		archive << array_mode() << pair.first << pair.second;
	}
	template<detail::pair_like T, typename A>
	void deserialize(T &pair, A &archive) requires(!detail::tuple_like<T>)
	{
		archive >> pair.first >> pair.second;
	}
}	 // namespace sek::serialization