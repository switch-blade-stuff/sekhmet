//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include <concepts>
#include <ranges>

#include "../manipulators.hpp"

namespace sek::serialization
{
	template<typename A, std::ranges::forward_range R>
	void serialize(const R &range, A &archive)
	{
		if constexpr (std::ranges::sized_range<R>)
			archive << array_entry{std::ranges::size(range)};
		else
			archive << array_entry{dynamic_size};
		for (auto &item : range) archive << item;
	}

	template<typename A, std::ranges::forward_range R>
	void deserialize(R &r, A &a) requires(requires(std::ranges::range_value_t<R> &value) { r.push_back(value); })
	{
		if constexpr (fixed_size_archive<A> && requires { r.reserve(std::ranges::range_size_t<R>{}); })
		{
			std::ranges::range_size_t<R> size;
			a >> array_entry{size};
			r.reserve(size);
		}

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
	void deserialize(R &range, A &archive)
	{
		auto range_item = std::ranges::begin(range), range_end = std::ranges::end(range);
		if constexpr (container_like_archive<A>)
		{
			auto archive_entry = archive.begin(), archive_end = archive.end();
			for (; range_item != range_end && archive_entry != archive_end; ++range_item, ++archive_entry)
				archive_entry->read(*range_item);
		}
		else
			for (; range_item != range_end; ++range_item)
			{
				if (!archive.try_read(*range_item)) [[unlikely]]
					break;
			}
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
}	 // namespace sek::serialization