/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include <stdexcept>

#include "sekhmet/detail/define.h"

namespace sek
{
	/** @brief Exception thrown by the asset system on runtime errors. */
	class SEK_API asset_error : public std::runtime_error
	{
	public:
		asset_error() : std::runtime_error("Unknown asset error") {}
		explicit asset_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit asset_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit asset_error(const char *msg) : std::runtime_error(msg) {}
		~asset_error() override;
	};
}	 // namespace sek