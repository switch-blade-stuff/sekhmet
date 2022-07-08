/*
 * Created by switchblade on 07/07/22
 */

#include "config.hpp"

#include "ctype.h"

template class SEK_API_EXPORT sek::service<sek::engine::detail::config_guard>;

namespace sek::engine
{
	config_error::~config_error() = default;

	void cfg_path::parse()
	{
		/* There is always at least 1 element (the category). */
		m_slices.clear();
		m_slices.reserve(1);

		if (m_path.empty() || m_path.back() == '/') [[unlikely]]
			throw config_error("Config path category must not be empty");

		const auto loc = std::locale{};
		for (std::size_t base = 0, next = 0; next < m_path.size();)
		{
			if (const auto c = m_path[next]; c == '/') [[unlikely]]
			{
				/* Strip terminating & repeating slashes. */
				if (next + 1 == m_path.size()) [[unlikely]]
					break;
				else if (m_path[next + 1] == c) [[unlikely]]
				{
					m_path.erase(next);
					continue;
				}
				base = next++;
				m_slices.emplace_back(slice_t{base, next});
			}
			else
			{
				if (!std::isalnum(c, loc)) [[unlikely]]
					switch (c)
					{
						case '-':
						case '_':
						case '.': break;
						default: throw config_error(std::string{"Invalid config path character"} + c);
					}
				next = ++m_slices.back().last;
			}
		}
	}
	cfg_path cfg_path::to_component(const slice_t *first, const slice_t *last) const
	{
		cfg_path result;
		result.m_slices.reserve(static_cast<std::size_t>(std::distance(first, last)));
		for (;;)
		{
			auto slice = *first;
			result.m_slices.push_back(slice);
			result.m_path.append(m_path.data() + slice.first, m_path.data() + slice.last);

			if (++first == last) [[unlikely]]
				break;
			result.m_path.append(1, '/');
		}
		return result;
	}
}	 // namespace sek::engine
