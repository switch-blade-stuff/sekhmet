//
// Created by switch_blade on 2022-10-04.
//

#include "type_info.hpp"

namespace sek
{
	bool type_info::has_parent(type_info other) const noexcept
	{
		if (valid() && other.valid()) [[likely]]
			for (auto &parent : m_data->parents)
			{
				const auto parent_type = type_info{parent.type};
				if (parent_type == other || parent_type.has_parent(other)) [[likely]]
					return true;
			}
		return false;
	}
}	 // namespace sek