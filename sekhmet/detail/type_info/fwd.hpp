//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "../../define.h"
#include <initializer_list>

namespace sek
{
	namespace detail
	{
		template<auto>
		struct func_traits;

		struct type_data;
		struct type_handle;
		struct tuple_type_data;
		struct range_type_data;
		struct table_type_data;
	}	 // namespace detail

	class type_error;
	enum class type_errc;

	class type_info;
	class type_query;
	class type_database;

	class any;
	class any_ref;
	class any_tuple;
	class any_range;
	class any_table;
	class any_string;

	template<typename T>
	[[nodiscard]] any forward_any(T &&);
	template<typename T, typename... Args>
	[[nodiscard]] any make_any(Args &&...);
	template<typename T, typename U, typename... Args>
	[[nodiscard]] any make_any(std::initializer_list<U>, Args &&...);
}	 // namespace sek