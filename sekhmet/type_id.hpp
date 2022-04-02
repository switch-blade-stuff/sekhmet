//
// Created by switchblade on 2022-01-26.
//

#pragma once

#include "detail/type_id.hpp"

namespace sek::literals
{
	[[nodiscard]] constexpr type_id operator""_tid(const char *str, std::size_t len) noexcept
	{
		return type_id{{str, len}};
	}
}	 // namespace sek::literals