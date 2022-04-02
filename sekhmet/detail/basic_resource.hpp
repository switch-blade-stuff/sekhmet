//
// Created by switchblade on 2022-04-01.
//

#pragma once

#include <iostream>

#include "sekhmet/detail/adt/node.hpp"

namespace sek
{
	class resource_db;

	namespace detail
	{
		struct resource_base
		{
			constexpr resource_base() noexcept = default;
			constexpr virtual ~resource_base() = default;
		};
	}	 // namespace detail

	/** @brief Base interface for resource types. */
	template<typename Child>
	class basic_resource : virtual detail::resource_base
	{
		friend class resource_db;

		constexpr static Child make_instance(std::istream &data_stream) noexcept
		{
			static_assert(std::is_destructible_v<Child>, "Resource type must have a public destructor");
			/* If the child resource has an istream constructor, it will do deserialization on its own. */
			if constexpr (std::is_constructible_v<Child, std::istream &>)
				return Child{data_stream};
			else
			{
				adt::node

			}
		}

	public:
		constexpr basic_resource() noexcept = default;
		constexpr ~basic_resource() override = default;
	};
}	 // namespace sek