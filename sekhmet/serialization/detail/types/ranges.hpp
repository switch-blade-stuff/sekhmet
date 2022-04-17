//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include <concepts>
#include <ranges>

#include "../manipulators.hpp"

namespace sek::serialization
{
	namespace detail
	{
		template<typename R>
		concept has_push_back = requires(R &r, std::ranges::range_value_t<R> &value)
		{
			r.push_back(value);
		};
		template<typename R>
		concept has_end_insert = requires(R &r, std::ranges::range_value_t<R> &value)
		{
			r.insert(r.end(), value);
		};

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
		concept map_like = requires(R &r)
		{
			std::ranges::forward_range<R>;
			has_end_insert<R>;
			typename R::key_type;
			typename R::mapped_type;
			typename R::value_type;

			// clang-format off
			requires requires(typename R::value_type &v)
			{
				v.first;
				std::same_as<std::decay_t<decltype(v.first)>, std::decay_t<typename R::key_type>>;
				v.second;
				std::same_as<std::decay_t<decltype(v.second)>, std::decay_t<typename R::mapped_type>>;
			};
			requires requires(typename R::key_type &&k, typename R::mapped_type &&m)
			{
				r.emplace(std::forward<typename R::key_type>(k), std::forward<typename R::mapped_type>(m));
			};
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

	template<typename A, std::ranges::forward_range R>
	void serialize(const R &range, A &archive)
	{
		archive << array_mode();
		if constexpr (std::ranges::sized_range<R>) archive << container_size(std::ranges::size(range));
		for (auto &item : range) archive << item;
	}

	template<typename A, typename T, std::size_t N>
	void deserialize(T (&data)[N], A &archive)
	{
		auto data_item = std::ranges::begin(data), data_end = std::ranges::end(data);
		if constexpr (container_like_archive<A>)
		{
			auto archive_entry = archive.begin(), archive_end = archive.end();
			for (; data_item != data_end && archive_entry != archive_end; ++data_item, ++archive_entry)
				archive_entry->read(*data_item);
		}
		else
			for (; data_item != data_end; ++data_item)
			{
				if (!archive.try_read(*data_item)) [[unlikely]]
					break;
			}
	}

	template<typename A, std::ranges::forward_range M>
	void deserialize(M &m, A &a) requires detail::map_like<M>
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

		if constexpr (container_like_archive<A>)
			for (auto &entry : a)
			{
				auto value = entry.template read<value_t>();
				m.emplace(forward_key(value.key), forward_mapped(value.mapped));
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

	template<typename A, std::ranges::forward_range R>
	void deserialize(R &r, A &a) requires(!detail::map_like<R> && detail::has_push_back<R>)
	{
		detail::reserve_range(r, a);

		using V = std::ranges::range_value_t<R>;
		auto inserter = std::back_inserter(r);
		if constexpr (container_like_archive<A>)
			for (auto &entry : a) *inserter = entry.template read<V>();
		else
			for (;;)
			{

				V value;
				if (!a.try_read(value)) [[unlikely]]
					break;

				if constexpr (std::movable<V>)
					*inserter = std::move(value);
				else
					*inserter = value;
			}
	}
	template<typename A, std::ranges::forward_range R>
	void deserialize(R &r, A &a) requires(!detail::map_like<R> && !detail::has_push_back<R> && detail::has_end_insert<R>)
	{
		detail::reserve_range(r, a);

		using V = std::ranges::range_value_t<R>;
		if constexpr (container_like_archive<A>)
			for (auto &entry : a) r.insert(r.end(), entry.template read<V>());
		else
			for (;;)
			{
				V value;
				if (!a.try_read(value)) [[unlikely]]
					break;

				if constexpr (std::movable<V>)
					r.insert(r.end(), std::move(value));
				else
					r.insert(r.end(), value);
			}
	}
}	 // namespace sek::serialization