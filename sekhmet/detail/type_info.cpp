//
// Created by switchblade on 16/05/22.
//

#include "type_info.hpp"

namespace sek
{
	type_info::type_data *type_info::reflect_impl(type_handle handle)
	{
		return handle.instance();
	}
}