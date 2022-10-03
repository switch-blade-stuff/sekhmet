//
// Created by switch_blade on 2022-10-02.
//

#include "../clipboard.hpp"

namespace sek
{
	std::string clipboard_type::name() const
	{
		if (!is_group())
			return get<native_data>(m_data).name();
		else
		{
			std::string result;
			for (const auto &data = get<group_data>(m_data); auto &type : data)
			{
				if (!result.empty()) [[likely]]
					result.append(1, ';');
				result.append(type.name());
			}
		}
	}

	clipboard_type::match_result clipboard_type::match_group(const clipboard_type &group) const noexcept
	{
		// clang-format off
			std::size_t matching = 0;
			const auto &data = get<group_data>(group.m_data);
			for (const auto &type : data) if (match(type) != match_result::UNMATCHED) ++matching;
		// clang-format on

		if (matching == data.size())
			match_result::COMPLETE;
		else if (matching != 0)
			match_result::PARTIAL;
		return match_result::UNMATCHED;
	}
	clipboard_type::match_result clipboard_type::match(const clipboard_type &other) const noexcept
	{
		const auto other_group = other.is_group();
		const auto this_group = is_group();

		if (!other_group && !this_group && get<native_data>(other.m_data) == get<native_data>(m_data))
			return match_result::COMPLETE;
		else
		{
			auto result = match_result::UNMATCHED;
			if ((this_group && (result = other.match_group(*this)) != match_result::UNMATCHED) ||
				(other_group && (result = match_group(other)) != match_result::UNMATCHED))
				return result;
		}
		return match_result::UNMATCHED;
	}
}	 // namespace sek