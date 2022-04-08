//
// Created by switchblade on 2022-02-13.
//

#pragma once

#include <ranges>

#include "node.hpp"

namespace sek::adt
{
	namespace detail
	{
		template<typename T>
		concept serializable_type = std::constructible_from<node, T> || requires(node &n, const T &v)
		{
			n.set(v);
		};
		template<typename T>
		concept move_serializable_type = std::constructible_from<node, T &&> || requires(node &n, T &&v)
		{
			n.set(std::move(v));
		};
		template<typename T>
		concept deserializable_type = requires(const node &n, T &v)
		{
			n.get(v);
		};
		template<typename T>
		concept move_deserializable_type = requires(node &&n, T &v)
		{
			std::move(n).get(v);
		};

		template<typename T>
		concept pair_like_serializable = requires(T t)
		{
			t.first;
			serializable_type<decltype(t.first)>;
			t.second;
			serializable_type<decltype(t.second)>;
		};
		template<typename T>
		concept pair_like_move_serializable = requires(T t)
		{
			pair_like_serializable<T>;
			move_serializable_type<decltype(t.first)> || move_serializable_type<decltype(t.second)>;
		};
		template<typename T>
		concept pair_like_deserializable = requires(T t)
		{
			t.first;
			deserializable_type<decltype(t.first)>;
			t.second;
			deserializable_type<decltype(t.second)>;
		};
		template<typename T>
		concept pair_like_move_deserialize = requires(T t)
		{
			pair_like_deserializable<T>;
			move_deserializable_type<decltype(t.first)> || move_deserializable_type<decltype(t.second)>;
		};

		template<typename T>
		concept array_like_serializable = requires
		{
			std::ranges::forward_range<T>;
			serializable_type<std::ranges::range_value_t<T>>;
		};
		template<typename T>
		concept array_like_move_serializable = requires
		{
			array_like_serializable<T>;
			move_serializable_type<std::ranges::range_value_t<T>>;
		};
		template<typename T>
		concept array_like_deserializable = requires
		{
			std::ranges::forward_range<T>;
			deserializable_type<std::ranges::range_value_t<T>>;
		};
		template<typename T>
		concept array_like_move_deserialize = requires
		{
			array_like_deserializable<T>;
			move_deserializable_type<std::ranges::range_value_t<T>>;
		};

		template<typename T>
		concept table_like_serializable = requires(T t)
		{
			typename T::key_type;
			typename T::mapped_type;
			std::ranges::forward_range<T>;
			serializable_type<typename T::mapped_type>;
			std::constructible_from<typename node::string_type, const typename T::key_type &>;
		};
		template<typename T>
		concept table_like_deserializable = requires(T t)
		{
			typename T::key_type;
			typename T::mapped_type;
			{
				t[std::declval<typename T::key_type>()]
				} -> std::same_as<typename T::mapped_type &>;
			std::ranges::forward_range<T>;
			deserializable_type<typename T::mapped_type>;
			std::constructible_from<typename T::key_type, const typename node::string_type &>;
		};
		template<typename T>
		concept table_like_move_deserializable = requires(T t)
		{
			table_like_deserializable<T>;
			move_deserializable_type<typename T::mapped_type>;
		};
	}	 // namespace detail

	template<detail::pair_like_serializable P>
	constexpr void serialize(node &n, const P &value)
	{
		n = node{std::in_place_type<typename node::table_type>, 2u};
		auto &node_table = n.as_table();

		if constexpr (std::constructible_from<node, decltype(value.first) &>)
			node_table.emplace("first", value.first);
		else
			node_table["first"].set(value.first);

		if constexpr (std::constructible_from<node, decltype(value.second) &>)
			node_table.emplace("second", value.second);
		else
			node_table["second"].set(value.second);
	}
	template<detail::pair_like_move_serializable P>
	constexpr void serialize(node &n, P &&value)
	{
		n = node{std::in_place_type<typename node::table_type>, 2u};
		auto &node_table = n.as_table();

		if constexpr (std::constructible_from<node, decltype(value.first) &&>)
			node_table.emplace("first", std::move(value.first));
		else
			node_table["first"].set(std::move(value.first));

		if constexpr (std::constructible_from<node, decltype(value.second) &&>)
			node_table.emplace("second", std::move(value.second));
		else
			node_table["second"].set(std::move(value.second));
	}
	template<detail::pair_like_deserializable P>
	constexpr void deserialize(const node &n, P &value)
	{
		if (n.is_table()) [[likely]]
		{
			auto &table = n.as_table();
			if (auto iter = table.find("first"); iter != table.end()) [[likely]]
				iter->second.get(value.first);
			if (auto iter = table.find("second"); iter != table.end()) [[likely]]
				iter->second.get(value.second);
		}
	}
	template<detail::pair_like_move_deserialize P>
	constexpr void deserialize(node &&n, P &value)
	{
		if (n.is_table()) [[likely]]
		{
			auto &table = n.as_table();
			if (auto iter = table.find("first"); iter != table.end()) [[likely]]
				std::move(iter->second).get(value.first);
			if (auto iter = table.find("second"); iter != table.end()) [[likely]]
				std::move(iter->second).get(value.second);
		}
	}

	template<detail::array_like_serializable R>
	void serialize(node &n, const R &value)
	{
		n = node{std::in_place_type<typename node::sequence_type>};
		auto &node_array = n.as_sequence();

		if constexpr (std::constructible_from<node, std::ranges::range_value_t<R>>)
			node_array.insert(node_array.end(), value.begin(), value.end());
		else
			for (auto &item : value)
			{
				node_array.emplace_back();
				node_array.back().set(item);
			}
	}
	template<detail::array_like_move_serializable R>
	void serialize(node &n, R &&value)
	{
		n = adt::sequence{};
		auto &node_array = n.as_sequence();

		if constexpr (std::constructible_from<node, std::ranges::range_value_t<R>>)
			node_array.insert(node_array.end(), std::make_move_iterator(value.begin()), std::make_move_iterator(value.end()));
		else
			for (auto &item : value)
			{
				node_array.emplace_back();
				node_array.back().set(std::move(item));
			}
	}
	template<detail::array_like_deserializable R>
	void deserialize(const node &n, R &value)
	{
		if (n.is_sequence()) [[likely]]
		{
			for (auto out_iter = std::inserter(value, std::ranges::end(value)); auto &item : n.as_sequence())
				*out_iter++ = item.get<std::ranges::range_value_t<R>>();
		}
	}
	template<detail::array_like_move_deserialize R>
	void deserialize(node &&n, R &value)
	{
		if (n.is_sequence()) [[likely]]
		{
			for (auto out_iter = std::inserter(value, std::ranges::end(value)); auto &item : n.as_sequence())
				*out_iter++ = std::move(item).get<std::ranges::range_value_t<R>>();
		}
	}

	template<detail::table_like_serializable T>
	void serialize(node &n, const T &value)
	{
		n = node{std::in_place_type<typename node::table_type>, value.size()};
		for (auto &node_table = n.as_table(); auto &pair : value)
		{
			if constexpr (std::constructible_from<node, decltype(pair.first) &>)
				node_table.emplace(pair.first, pair.second);
			else
				node_table[pair.first].set(pair.second);
		}
	}
	template<detail::table_like_deserializable T>
	void deserialize(const node &n, T &value)
	{
		if (n.is_table()) [[likely]]
			for (auto &pair : n.as_table()) pair.second.get(value[pair.first]);
	}
	template<detail::table_like_move_deserializable T>
	void deserialize(node &&n, T &value)
	{
		if (n.is_table()) [[likely]]
			for (auto &pair : n.as_table()) std::move(pair.second).get(value[pair.first]);
	}
}	 // namespace sek::adt
