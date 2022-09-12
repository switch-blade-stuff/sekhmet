/*
 * Created by switchblade on 05/09/22
 */

#include "json_error.hpp"

#include <fmt/format.h>

namespace sek::serialization
{
	json_error::~json_error() = default;

	[[nodiscard]] constexpr static std::string_view type_string(json_type type) noexcept
	{
		switch (type)
		{
			case json_type::CONTAINER_FLAG: return "container";
			case json_type::NUMBER_FLAG: return "number";
			case json_type::NULL_VALUE: return "null";
			case json_type::INT: return "int";
			case json_type::UINT: return "uint";
			case json_type::FLOAT: return "float";
			case json_type::ARRAY: return "array";
			case json_type::TABLE: return "table";
			case json_type::STRING: return "string";
			default: return "unknown";
		}
	}

	void detail::invalid_json_type(json_type expected, json_type actual)
	{
		const auto expected_str = type_string(expected);
		const auto actual_str = type_string(actual);
		throw json_error(fmt::format("Invalid Json type, expected <{}>, actual <{}>.", expected_str, actual_str));
	}
}	 // namespace sek::serialization