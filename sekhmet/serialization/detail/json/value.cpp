/*
 * Created by switchblade on 05/09/22
 */

#include "value.hpp"

#include <fmt/format.h>

namespace sek::serialization
{
	json_error::~json_error() = default;

	void detail::throw_invalid_type(json_type expected, json_type actual)
	{
		const auto expected_str = type_string(expected);
		const auto actual_str = type_string(actual);
		throw json_error(fmt::format("Invalid Json type, expected <{}>, actual <{}>.", expected_str, actual_str));
	}
	void detail::throw_invalid_type(json_type expected)
	{
		const auto expected_str = type_string(expected);
		throw json_error(fmt::format("Invalid Json type, expected <{}>.", expected_str));
	}
}	 // namespace sek::serialization