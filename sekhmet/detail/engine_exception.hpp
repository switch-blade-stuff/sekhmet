//
// Created by switchblade on 2021-10-13.
//

#pragma once

#include <exception>

#include "define.h"

namespace sek
{
	/** Base abstract class for generic sekhmet exceptions. */
	class engine_exception : public std::exception
	{
	public:
		engine_exception() noexcept = default;
		~engine_exception() noexcept override = default;

		[[nodiscard]] const char *what() const noexcept override = 0;
	};
}	 // namespace sek
