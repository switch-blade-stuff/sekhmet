/*
 * Created by switchblade on 05/09/22
 */

#pragma once

#include <stdexcept>

#include "sekhmet/detail/define.h"

namespace sek::serialization
{
	/** @brief Exception thrown by archives on runtime errors. */
	class SEK_API archive_error : public std::runtime_error
	{
	public:
		archive_error() : std::runtime_error("Unknown archive error") {}
		explicit archive_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit archive_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit archive_error(const char *msg) : std::runtime_error(msg) {}
		~archive_error() override;
	};

	/* TODO: Refactor `archive_error` to derive from `std::system_error`. */
	/* TODO: Implement generic archive error category. */
}	 // namespace sek::serialization