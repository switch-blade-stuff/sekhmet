/*
 * Created by switchblade on 07/07/22
 */

#include "config.hpp"

#include <ctype.h>

template class SEK_API_EXPORT sek::service<sek::engine::detail::config_guard>;

namespace sek::engine
{
	config_error::~config_error() = default;

	void cfg_path::parse()
	{
		/* There is always at least 1 element (the category). */
		m_slices.clear();
		m_slices.reserve(1);

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
				if (!(std::isalnum(c, loc) || std::isspace(c, loc))) [[unlikely]]
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

	typename config_registry::iterator config_registry::find(const cfg_path &path)
	{
		return iterator{m_entries.find(path.string())};
	}
	typename config_registry::const_iterator config_registry::find(const cfg_path &path) const
	{
		return const_iterator{m_entries.find(path.string())};
	}

	config_registry::entry_node *config_registry::insert_impl(cfg_path &&path)
	{
		struct node_deleter
		{
			void operator()(entry_node *node) const
			{
				std::destroy_at(node);
				reg.m_node_pool.deallocate(node);
			}

			config_registry &reg;
		};

		const auto add_category = path.is_category();
		const auto node = std::construct_at(m_node_pool.allocate(), std::forward<cfg_path>(path));
		try
		{
			/* Create the new node. */
			if (add_category) [[unlikely]]
				m_categories.try_emplace(node->path.string(), node);
			return m_entries.try_emplace(node->path.string(), node).first->second;
		}
		catch (...)
		{
			std::destroy_at(node);
			m_node_pool.deallocate(node);
			throw;
		}
	}
	void config_registry::insert_impl(cfg_path &&path, entry_value *value)
	{
		/* Insert the new node. */
		entry_node *node;
		try
		{
			node = insert_impl(std::forward<cfg_path>(path));
		}
		catch (...)
		{
			std::destroy_at(value);
			m_value_pool.deallocate(value);
			throw;
		}
		node->value = value;

		/* Unwind & create "dummy" entries for every parent path. */
		entry_node *child_node = node, *parent_node;
		auto &child_path = node->path;
		while (child_path.is_category())
		{
			{ /* Find or create parent node. */
				auto parent_path = child_path.parent_path();
				if (auto parent_iter = find(parent_path); parent_iter == end())
					parent_node = insert_impl(std::move(parent_path));
				else
					parent_node = parent_iter.m_iter->second;
			}

			/* Insert child node into the parent. */
			parent_node->children.try_emplace(child_path.string(), child_node);

			child_node = parent_node;
			child_path = parent_node->path;
		}
	}

	void config_registry::erase(const_iterator where)
	{
		/* Erase all child nodes. */
		for (auto child = where->begin(), end = where->end(); child != end; ++child) erase(child);

		/* Erase the current node. */
		const auto node = where.m_iter->second;
		const auto value = node->value;

		std::destroy_at(node);
		std::destroy_at(value);
		m_node_pool.deallocate(node);
		m_node_pool.deallocate(value);
	}
}	 // namespace sek::engine
