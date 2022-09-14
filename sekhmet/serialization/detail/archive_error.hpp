/*
 * Created by switchblade on 05/09/22
 */

#pragma once

#include "sekhmet/detail/define.h"

#include <system_error>

namespace sek::serialization
{
	/** @brief Exception thrown by serialization archives on runtime errors. */
	class SEK_API archive_error : public std::system_error
	{
	public:
		archive_error(const archive_error &) noexcept = default;

		archive_error(std::error_code ec) : std::system_error(ec) {}
		archive_error(std::error_code ec, const char *msg) : std::system_error(ec, msg) {}
		archive_error(std::error_code ec, const std::string &msg) : std::system_error(ec, msg) {}

		archive_error(int ev, const std::error_category &c) : std::system_error(ev, c) {}
		archive_error(int ev, const std::error_category &c, const char *msg) : std::system_error(ev, c, msg) {}
		archive_error(int ev, const std::error_category &c, const std::string &msg) : std::system_error(ev, c, msg) {}

		~archive_error() override;
	};

	/** @brief Error code used to specify general serialization archive errors. */
	enum class archive_errc : int
	{
		/** Invalid/unexpected data type. */
		INVALID_TYPE = 1,
		/** Failed to read the archive due to a premature end of input. */
		UNEXPECTED_END = 2,
	};

	/** Returns a reference to `std::error_category` used for generic serialization errors. */
	[[nodiscard]] SEK_API const std::error_category &archive_category() noexcept;

	/** Creates an instance of  `std::error_code` from the specified `archive_errc` value.
	 * Equivalent to `std::error_code{static_cast<int>(e), archive_category()}`. */
	[[nodiscard]] std::error_code make_error_code(archive_errc e) noexcept
	{
		return std::error_code{static_cast<int>(e), archive_category()};
	}
}	 // namespace sek::serialization