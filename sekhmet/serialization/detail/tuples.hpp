/*
 * Created by switchblade on 2022-04-16
 */

#pragma once

#include <concepts>
#include <tuple>
#include <utility>

#include "../../meta.hpp"
#include "manipulators.hpp"

namespace sek
{
	namespace detail
	{
		template<pair_like P>
		struct pair_traits
		{
			using first_type = std::remove_cvref_t<decltype(std::declval<P>().first)>;
			using second_type = std::remove_cvref_t<decltype(std::declval<P>().second)>;
		};

		template<detail::tuple_like T, typename A, typename... Args, std::size_t I, std::size_t... Is>
		void tuple_serialize_unwrap(std::index_sequence<I, Is...>, const T &t, A &a, Args &&...args)
		{
			using std::get;
			a.write(get<I>(t), std::forward<Args>(args)...);
			if constexpr (sizeof...(Is) != 0)
				tuple_serialize_unwrap(std::index_sequence<Is...>{}, t, a, std::forward<Args>(args)...);
		}
		template<detail::tuple_like T, typename A, typename... Args, std::size_t I, std::size_t... Is>
		void tuple_deserialize_unwrap(std::index_sequence<I, Is...>, T &t, A &a, Args &&...args)
		{
			using std::get;
			a.read(get<I>(t), std::forward<Args>(args)...);
			if constexpr (sizeof...(Is) != 0)
				tuple_deserialize_unwrap(std::index_sequence<Is...>{}, t, a, std::forward<Args>(args)...);
		}
	}	 // namespace detail

	// clang-format off
	template<typename T, typename A, typename... Args>
	void serialize(const T &tuple, A &archive, Args &&...args) requires(!std::ranges::forward_range<T> && detail::tuple_like<T>)
	{
		if constexpr (std::tuple_size_v<T> != 0)
		{
			archive << array_mode() << container_size(std::tuple_size_v<T>);
			detail::tuple_serialize_unwrap(
				std::make_index_sequence<std::tuple_size_v<T>>{}, tuple, archive, std::forward<Args>(args)...);
		}
	}
	template<typename T, typename A, typename... Args>
	void deserialize(T &tuple, A &archive, Args &&...args) requires(!std::ranges::forward_range<T> && detail::tuple_like<T>)
	{
		if constexpr (std::tuple_size_v<T> != 0)
			detail::tuple_deserialize_unwrap(std::make_index_sequence<std::tuple_size_v<T>>{}, tuple, archive, std::forward<Args>(args)...);
	}

	template<detail::pair_like T, typename A, typename... Args>
	void serialize(const T &pair, A &archive, Args &&...args) requires(!detail::tuple_like<T>)
	{
		/* Treat pairs as dynamic-size arrays, since in most cases using a fixed size will have more overhead (need to store a size). */
		archive << array_mode();
		archive.write(pair.first, std::forward<Args>(args)...);
		archive.write(pair.second, std::forward<Args>(args)...);
	}
	template<detail::pair_like T, typename A, typename... Args>
	void deserialize(T &pair, A &archive, Args &&...args) requires(!detail::tuple_like<T>)
	{
		archive.read(pair.first, std::forward<Args>(args)...);
		archive.read(pair.second, std::forward<Args>(args)...);
	}
	// clang-format on
}	 // namespace sek