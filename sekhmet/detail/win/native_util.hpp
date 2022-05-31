//
// Created by switchblade on 30/05/22.
//

#pragma once

namespace sek::detail
{
	constexpr auto native_in = static_cast<native_openmode>(1);
	constexpr auto native_out = static_cast<native_openmode>(2);
	constexpr auto native_copy = static_cast<native_openmode>(4);
	constexpr auto native_append = static_cast<native_openmode>(8);
	constexpr auto native_create = static_cast<native_openmode>(16);
	constexpr auto native_trunc = static_cast<native_openmode>(32);
}	 // namespace sek::detail