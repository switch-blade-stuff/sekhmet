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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2022-04-16
 */

#pragma once

#include <concepts>
#include <ranges>

#include "../manipulators.hpp"

namespace sek::serialization
{
	namespace detail
	{
		template<typename R>
		concept has_push_back = requires(R &r, std::ranges::range_value_t<R> &value) { r.push_back(value); };
		template<typename R>
		concept has_end_insert = requires(R &r, std::ranges::range_value_t<R> &value) { r.insert(r.end(), value); };

		template<typename A, typename R>
		constexpr void reserve_range(R &r, A &a)
		{
			if constexpr (requires { r.reserve(std::ranges::range_size_t<R>{}); })
			{
				std::ranges::range_size_t<R> size = 0;
				a >> container_size(size);
				r.reserve(size);
			}
		}

		template<typename R>
		concept map_like =
			requires(R &r) {
				requires std::ranges::forward_range<R>;
				typename R::key_type;
				typename R::mapped_type;
				typename R::value_type;

				// clang-format off
				requires requires(typename R::value_type &v)
				{
					v.first;
					v.second;
					requires std::same_as<std::decay_t<decltype(v.first)>, std::decay_t<typename R::key_type>>;
					requires std::same_as<std::decay_t<decltype(v.second)>, std::decay_t<typename R::mapped_type>>;
				};
				requires requires(typename R::key_type &&k, typename R::mapped_type &&m)
				{
					r.emplace(std::forward<typename R::key_type>(k), std::forward<typename R::mapped_type>(m));
				};
				// clang-format on
			};
		template<typename R>
		concept object_like = requires {
								  // clang-format off
								  requires map_like<R>;
								  requires requires(const typename R::value_type &v) { sek::serialization::keyed_entry(v.first, v.second); };
								  // clang-format on
							  };

		template<typename K, typename M>
		struct map_entry
		{
			void serialize(auto &a) { a << array_mode() << key << mapped; }
			void deserialize(auto &a) { a >> key >> mapped; }

			K key;
			M mapped;
		};
	}	 // namespace detail

	template<typename T, std::size_t N, typename A, typename... Args>
	void deserialize(T (&data)[N], A &archive, Args &&...args)
	{
		auto data_item = std::ranges::begin(data), data_end = std::ranges::end(data);
		if constexpr (structured_data_archive<A>)
		{
			auto archive_entry = archive.begin(), archive_end = archive.end();
			for (; data_item != data_end && archive_entry != archive_end; ++data_item, ++archive_entry)
				archive_entry->read(*data_item, std::forward<Args>(args)...);
		}
		else
			for (; data_item != data_end; ++data_item)
			{
				if (!archive.try_read(*data_item, std::forward<Args>(args)...)) [[unlikely]]
					break;
			}
	}
	template<std::ranges::forward_range T, typename A, typename... Args>
	void deserialize(T &array, A &archive, Args &&...args)
		requires(requires { typename std::tuple_size<T>::type; })
	{
		auto data_item = std::ranges::begin(array), data_end = std::ranges::end(array);
		if constexpr (structured_data_archive<A>)
		{
			auto archive_entry = archive.begin(), archive_end = archive.end();
			for (; data_item != data_end && archive_entry != archive_end; ++data_item, ++archive_entry)
				archive_entry->read(*data_item, std::forward<Args>(args)...);
		}
		else
			for (; data_item != data_end; ++data_item)
			{
				if (!archive.try_read(*data_item, std::forward<Args>(args)...)) [[unlikely]]
					break;
			}
	}

	template<std::ranges::forward_range M, typename A, typename... Args>
	void deserialize(M &m, A &a, Args &&...args)
		requires detail::map_like<M>
	{
		/* Dummy used to read value pairs, since maps usually have const keys. */
		using value_t = detail::map_entry<typename M::key_type, typename M::mapped_type>;

		constexpr auto forward_key = [](typename M::key_type &key) -> decltype(auto)
		{
			if constexpr (std::movable<typename M::key_type>)
				return std::move(key);
			else
				return key;
		};
		constexpr auto forward_mapped = [](typename M::mapped_type &mapped) -> decltype(auto)
		{
			if constexpr (std::movable<typename M::mapped_type>)
				return std::move(mapped);
			else
				return mapped;
		};

		detail::reserve_range(m, a);

		if constexpr (structured_data_archive<A>)
		{
			std::size_t idx = 0;
			auto entry_iter = a.begin();
			auto archive_end = a.end();
			for (; entry_iter != archive_end; ++entry_iter)
			{
				// clang-format off
				constexpr auto keyed_entry = requires{ { entry_iter.has_key() } -> std::convertible_to<bool>; entry_iter.key(); };

				if constexpr (keyed_entry && requires { m[entry_iter.key()] = entry_iter->read(std::in_place_type<typename M::mapped_type>, std::forward<Args>(args)...); })
				{
					if (entry_iter.has_key())
						entry_iter->template read<typename M::mapped_type>(m[entry_iter.key()], std::forward<Args>(args)...);
					else
						entry_iter->template read<typename M::mapped_type>(m[std::to_string(idx++)], std::forward<Args>(args)...);
				}
				else if constexpr (keyed_entry && requires { m.emplace(entry_iter.key(), entry_iter->read(std::in_place_type<typename M::mapped_type>, std::forward<Args>(args)...)); })
				{
					if (entry_iter.has_key())
						m.emplace(entry_iter.key(), std::move(entry_iter->read(std::in_place_type<typename M::mapped_type>, std::forward<Args>(args)...)));
					else
						m.emplace(std::to_string(idx++), std::move(entry_iter->read(std::in_place_type<typename M::mapped_type>, std::forward<Args>(args)...)));
				}
				else
				{
					auto value = archive_end->read(std::in_place_type<value_t>, std::forward<Args>(args)...);
					m.emplace(forward_key(value.key), forward_mapped(value.mapped));
				}
				// clang-format on
			}
		}
		else
			for (;;)
			{
				value_t value;
				if (!a.try_read(value)) [[unlikely]]
					break;
				m.emplace(forward_key(value.key), forward_mapped(value.mapped));
			}
	}
	template<std::ranges::forward_range M, typename A, typename... Args>
	void serialize(const M &m, A &a, Args &&...args)
		requires detail::object_like<M>
	{
		// clang-format off
		if constexpr (std::ranges::sized_range<M>)
			a.write(container_size(std::ranges::size(m)), std::forward<Args>(args)...);
		for (decltype(auto) pair : m)
			a.write(keyed_entry(pair.first, pair.second), std::forward<Args>(args)...);
		// clang-format on
	}

	template<std::ranges::forward_range R, typename A, typename... Args>
	void deserialize(R &r, A &a, Args &&...args)
		requires(!detail::map_like<R> && detail::has_push_back<R>)
	{
		detail::reserve_range(r, a);

		using V = std::ranges::range_value_t<R>;
		auto inserter = std::back_inserter(r);
		if constexpr (structured_data_archive<A>)
			for (decltype(auto) entry : a) *inserter = entry.read(std::in_place_type<V>, std::forward<Args>(args)...);
		else
			for (;;)
			{
				V value;
				if (!a.try_read(value, std::forward<Args>(args)...)) [[unlikely]]
					break;

				if constexpr (std::movable<V>)
					*inserter = std::move(value);
				else
					*inserter = value;
			}
	}
	template<std::ranges::forward_range R, typename A, typename... Args>
	void deserialize(R &r, A &a, Args &&...args)
		requires(!detail::map_like<R> && !detail::has_push_back<R> && detail::has_end_insert<R>)
	{
		detail::reserve_range(r, a);

		using V = std::ranges::range_value_t<R>;
		if constexpr (structured_data_archive<A>)
		{
			// clang-format off
			for (decltype(auto) entry : a)
				r.insert(r.end(), entry.read(std::in_place_type<V>, std::forward<Args>(args)...));
			// clang-format on
		}
		else
			for (;;)
			{
				V value;
				if (!a.try_read(value, std::forward<Args>(args)...)) [[unlikely]]
					break;

				if constexpr (std::movable<V>)
					r.insert(r.end(), std::move(value));
				else
					r.insert(r.end(), value);
			}
	}

	template<std::ranges::forward_range R, typename A, typename... Args>
	void serialize(const R &range, A &archive, Args &&...args)
	{
		archive << array_mode();
		// clang-format off
		if constexpr (std::ranges::sized_range<R>)
			archive << container_size(std::ranges::size(range));

		for (decltype(auto) item : range)
			archive.write(item, std::forward<Args>(args)...);
		// clang-format on
	}
}	 // namespace sek::serialization