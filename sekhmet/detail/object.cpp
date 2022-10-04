//
// Created by switch_blade on 2022-10-04.
//

#include "../object.hpp"

#include <fmt/format.h>

namespace sek
{
	object::~object() = default;

	object::cast_status object::check_cast(type_info from, type_info to) noexcept
	{
		if (from == to)
			return cast_status::SAME_TYPE;
		else if (from.has_parent(to))
			return cast_status::CHILD_TYPE;
		else if (to.has_parent(from))
			return cast_status::BASE_TYPE;
		return cast_status::UNRELATED;
	}
	const object *object::checked_ptr_cast(const object *ptr, type_info from, type_info to) noexcept
	{
		switch (check_cast(from, to))
		{
			case cast_status::SAME_TYPE: [[fallthrough]];
			case cast_status::CHILD_TYPE: return ptr;
			default: return nullptr;
		}
	}
	const object &object::checked_ref_cast(const object &ref, type_info from, type_info to)
	{
		std::string msg;
		switch (check_cast(from, to))
		{
			case cast_status::SAME_TYPE: [[fallthrough]];
			case cast_status::CHILD_TYPE: return ref;
			case cast_status::BASE_TYPE:
				msg = fmt::format("Cannot cast a parent object <{}> to a child type <{}>", from.name(), to.name());
				break;
			case cast_status::UNRELATED:
				msg = fmt::format("Cannot cast between unrelated types <{}> and <{}>", from.name(), to.name());
				break;
		}
		throw type_error(make_error_code(type_errc::INVALID_TYPE), msg);
	}
}	 // namespace sek