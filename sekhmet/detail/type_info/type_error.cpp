//
// Created by switch_blade on 2022-10-03.
//

#include "type_error.hpp"

#include <fmt/format.h>

namespace sek
{
	type_error::~type_error() = default;

	class type_category_t : public std::error_category
	{
	public:
		type_category_t() noexcept = default;
		~type_category_t() override = default;

		[[nodiscard]] const char *name() const noexcept override { return "type"; }
		[[nodiscard]] std::string message(int err) const override
		{
			const auto errc = static_cast<type_errc>(err);
			if ((errc & type_errc::INVALID_PARAM) != type_errc{})
				return param_message(errc);
			else if ((errc & type_errc::INVALID_TYPE) != type_errc{})
				return type_message(errc);
			return "Unknown error";
		}

		[[nodiscard]] std::string param_message(type_errc errc) const
		{
			const auto arg_idx = static_cast<std::uint8_t>(errc & type_errc::PARAM_MASK);
			if ((errc & type_errc::INVALID_TYPE) != type_errc{})
				return fmt::format("Invalid argument ({}): {}", arg_idx, type_message(errc));
			else
				return fmt::format("Invalid argument ({})", arg_idx);
		}
		[[nodiscard]] std::string type_message(type_errc errc) const
		{
			if ((errc & type_errc::INVALID_QUALIFIER) != type_errc{})
				return "Invalid type qualifier";
			else if ((errc & type_errc::INVALID_ATTRIBUTE) != type_errc{})
				return "Invalid type attribute";
			else if ((errc & type_errc::INVALID_MEMBER) != type_errc{})
				return mem_message(errc);
			return "Invalid type";
		}
		[[nodiscard]] std::string mem_message(type_errc errc) const
		{
			if ((errc & type_errc::INVALID_PROPERTY) != type_errc{})
				return "Invalid type member property";
			else if ((errc & type_errc::INVALID_FUNCTION) != type_errc{})
				return "Invalid type member function";
			else if ((errc & type_errc::INVALID_CONSTRUCTOR) != type_errc{})
				return "Invalid type constructor";
			else if ((errc & type_errc::INVALID_ENUMERATION) != type_errc{})
				return "Invalid type enumeration";
			return "Invalid type member";
		}
	};

	const std::error_category &type_category() noexcept
	{
		static const type_category_t instance;
		return instance;
	}
}	 // namespace sek