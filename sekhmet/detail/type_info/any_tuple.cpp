//
// Created by switch_blade on 2022-10-03.
//

#include "any_tuple.hpp"

#include <fmt/format.h>

namespace sek
{
	const detail::tuple_type_data *any_tuple::assert_data(const detail::type_data *data)
	{
		if (data->tuple_data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::INVALID_TYPE),
							 fmt::format("<{}> is not a tuple-like type", data->name));
		return data->tuple_data;
	}

	any any_tuple::get(size_type i) { return m_data->get(m_target, i); }
	any any_tuple::get(size_type i) const { return m_data->cget(m_target, i); }
}	 // namespace sek