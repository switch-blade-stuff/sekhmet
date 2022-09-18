/*
 * Created by switchblade on 05/09/22
 */

#include "json_error.hpp"

#include <fmt/format.h>

namespace sek::serialization
{
	void detail::invalid_json_type(json_type expected, json_type actual)
	{
		constexpr auto type_str = [](json_type type) noexcept -> std::string_view
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
		};
		const auto expected_str = type_str(expected);
		const auto actual_str = type_str(actual);

		auto msg = fmt::format("Invalid Json type, expected <{}>, actual <{}>.", expected_str, actual_str);
		throw archive_error{make_error_code(archive_errc::INVALID_TYPE), std::move(msg)};
	}

	class json_category_t : public std::error_category
	{
	public:
		constexpr json_category_t() noexcept = default;
		~json_category_t() override = default;

		[[nodiscard]] const char *name() const noexcept override { return "json"; }
		[[nodiscard]] std::string message(int err) const override
		{
			switch (static_cast<json_errc>(err))
			{
				default: return "Unknown error";
			}
		}
	};

	const std::error_category &json_category() noexcept
	{
		constinit static const json_category_t instance;
		return instance;
	}
}	 // namespace sek::serialization