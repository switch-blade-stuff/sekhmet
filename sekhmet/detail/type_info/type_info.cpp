//
// Created by switch_blade on 2022-10-04.
//

#include "type_info.hpp"

SEK_EXPORT_TYPE_INFO(sek::any)
SEK_EXPORT_TYPE_INFO(sek::any_ref)
SEK_EXPORT_TYPE_INFO(sek::type_info)

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

	bool type_info::has_attribute(type_info type) const noexcept
	{
		if (valid() && type.valid()) [[likely]]
		{
			for (auto &attr : m_data->attributes)
				if (type_info{attr.type} == type) return true;
		}
		return false;
	}

	bool type_info::has_constructor(std::span<const any> args) const noexcept
	{
		if (valid()) [[likely]]
		{
			if (args.empty() && m_data->default_ctor) /* Quick check for default ctor. */
				return true;

			for (auto &ctor : m_data->constructors)
			{
				constexpr auto match = [](const any &a, const detail::arg_type_data &b) -> bool
				{
					bool result = !b.is_const || a.is_const();
					return result && a.type() == type_info{b.type};
				};
				if (ctor.args.size() == args.size() && std::equal(args.begin(), args.end(), ctor.args.begin(), match))
					return true;
			}
		}
		return false;
	}
	bool type_info::has_constructor(std::span<const type_info> args) const noexcept
	{
		if (valid()) [[likely]]
		{
			if (args.empty() && m_data->default_ctor) /* Quick check for default ctor. */
				return true;

			for (auto &ctor : m_data->constructors)
			{
				constexpr auto match = [](type_info a, const detail::arg_type_data &b) { return a == type_info{b.type}; };
				if (ctor.args.size() == args.size() && std::equal(args.begin(), args.end(), ctor.args.begin(), match))
					return true;
			}
		}
		return false;
	}
}	 // namespace sek