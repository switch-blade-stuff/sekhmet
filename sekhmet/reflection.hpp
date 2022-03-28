//
// Created by switchblade on 2022-01-26.
//

#pragma once

#include "detail/any.hpp"
#include "detail/type_id.hpp"
#include "detail/type_info.hpp"

namespace sek
{
	using detail::hash;
	using detail::type_hash;
	using detail::type_id;
	using detail::type_name;

	using detail::type_info;

	using detail::any;
	using detail::any_ref;
	using detail::bad_type_exception;

	namespace literals
	{
		[[nodiscard]] constexpr type_id operator""_tid(const char *str, std::size_t len) noexcept
		{
			return type_id{{str, len}};
		}
	}	 // namespace literals
}	 // namespace sek