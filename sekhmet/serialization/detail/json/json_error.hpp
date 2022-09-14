/*
 * Created by switchblade on 11/09/22
 */

#pragma once

#include "../archive_error.hpp"
#include "type.hpp"

namespace sek::serialization
{
	/** @brief Exception thrown by Json archives, readers and writers. */
	class SEK_API json_error : public archive_error
	{
	public:
		json_error(const json_error &) noexcept = default;

		json_error(std::error_code ec) : archive_error(ec) {}
		json_error(std::error_code ec, const char *msg) : archive_error(ec, msg) {}
		json_error(std::error_code ec, const std::string &msg) : archive_error(ec, msg) {}

		json_error(int ev, const std::error_category &c) : archive_error(ev, c) {}
		json_error(int ev, const std::error_category &c, const char *msg) : archive_error(ev, c, msg) {}
		json_error(int ev, const std::error_category &c, const std::string &msg) : archive_error(ev, c, msg) {}

		~json_error() override;
	};

	namespace detail
	{
		[[noreturn]] SEK_API void invalid_json_type(json_type expected, json_type actual);
	}	 // namespace detail

	/** @brief Error code used to specify Json parsing errors. */
	enum class json_errc : int
	{
	};

	/** Returns a reference to `std::error_category` used for Json serialization errors. */
	[[nodiscard]] SEK_API const std::error_category &json_category() noexcept;

	/** Creates an instance of  `std::error_code` from the specified `json_errc` value.
	 * Equivalent to `std::error_code{static_cast<int>(e), json_category()}`. */
	[[nodiscard]] std::error_code make_error_code(json_errc e) noexcept
	{
		return std::error_code{static_cast<int>(e), json_category()};
	}
}	 // namespace sek::serialization
