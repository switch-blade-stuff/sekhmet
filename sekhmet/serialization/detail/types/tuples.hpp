/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2022-04-16
 */

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
		template<typename T>
		concept tuple_like = !std::is_reference_v<T> && requires(T t)
		{
			typename std::tuple_size<T>::type;
			requires std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
		};
		template<typename T>
		concept pair_like = requires(T &p)
		{
			p.first;
			p.second;
		};
		// clang-format on

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

	template<typename T, typename A, typename... Args>
	void serialize(const T &tuple, A &archive, Args &&...args)
		requires(!std::ranges::forward_range<T> && detail::tuple_like<T>)
	{
		if constexpr (std::tuple_size_v<T> != 0)
		{
			archive << array_mode() << container_size(std::tuple_size_v<T>);
			detail::tuple_serialize_unwrap(
				std::make_index_sequence<std::tuple_size_v<T>>{}, tuple, archive, std::forward<Args>(args)...);
		}
	}
	template<typename T, typename A, typename... Args>
	void deserialize(T &tuple, A &archive, Args &&...args)
		requires(!std::ranges::forward_range<T> && detail::tuple_like<T>)
	{
		if constexpr (std::tuple_size_v<T> != 0)
			detail::tuple_deserialize_unwrap(
				std::make_index_sequence<std::tuple_size_v<T>>{}, tuple, archive, std::forward<Args>(args)...);
	}

	template<detail::pair_like T, typename A, typename... Args>
	void serialize(const T &pair, A &archive, Args &&...args)
		requires(!detail::tuple_like<T>)
	{
		/* Treat pairs as dynamic-size arrays, since in most cases using a fixed size will have more overhead (need to store a size). */
		archive << array_mode();
		archive.write(pair.first, std::forward<Args>(args)...);
		archive.write(pair.second, std::forward<Args>(args)...);
	}
	template<detail::pair_like T, typename A, typename... Args>
	void deserialize(T &pair, A &archive, Args &&...args)
		requires(!detail::tuple_like<T>)
	{
		archive.read(pair.first, std::forward<Args>(args)...);
		archive.read(pair.second, std::forward<Args>(args)...);
	}
}	 // namespace sek::serialization