/*
 * Created by switchblade on 11/09/22
 */

#pragma once

#include "../archive_error.hpp"
#include "type.hpp"

namespace sek::serialization
{
	class SEK_API json_error_category : std::error_category
	{
		/* TODO: Implement Json error category. */
	};

	/** @brief Exception thrown by Json archives, readers and writers. */
	class SEK_API json_error : public archive_error
	{
	public:
		json_error() : archive_error("Unknown Json error") {}
		explicit json_error(std::string &&msg) : archive_error(std::move(msg)) {}
		explicit json_error(const std::string &msg) : archive_error(msg) {}
		explicit json_error(const char *msg) : archive_error(msg) {}
		~json_error() override;
	};

	namespace detail
	{
		[[noreturn]] SEK_API void invalid_json_type(json_type expected, json_type actual);
	}	 // namespace detail
}	 // namespace sek::serialization
