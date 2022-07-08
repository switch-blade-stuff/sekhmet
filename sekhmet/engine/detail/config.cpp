/*
 * Created by switchblade on 07/07/22
 */

#include "config.hpp"

#include <cctype>

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

	typename config_registry::entry_ptr<false> config_registry::find(const cfg_path &path)
	{
		if (const auto iter = m_entries.find(path.string()); iter != m_entries.end()) [[likely]]
			return entry_ptr<false>{*iter};
		return {};
	}
	typename config_registry::entry_ptr<true> config_registry::find(const cfg_path &path) const
	{
		if (const auto iter = m_entries.find(path.string()); iter != m_entries.end()) [[likely]]
			return entry_ptr<true>{*iter};
		return {};
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
			/* Create the new node & add new category if needed. */
			if (add_category) [[unlikely]]
				m_categories.try_insert(node);
			return *m_entries.try_insert(node).first;
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
				if (auto parent_ptr = find(parent_path); !parent_ptr)
					parent_node = insert_impl(std::move(parent_path));
				else
					parent_node = parent_ptr.m_ref.m_node;
			}

			/* Insert child node into the parent. */
			parent_node->children.insert(child_node);

			child_node = parent_node;
			child_path = parent_node->path;
		}
	}

	void config_registry::erase_impl(typename entry_set::const_iterator where)
	{
		const auto node = *where;

		/* Erase all child nodes. */
		for (auto child = node->children.end(), end = node->children.begin(); child-- != end;)
			erase_impl(m_entries.find(*child));

		/* Erase the node from the node table & category list. */
		if (auto iter = m_categories.find(node); iter != m_categories.end()) [[unlikely]]
			m_categories.erase(iter);
		m_entries.erase(where);

		/* Destroy the node & it's value */
		if (const auto value = node->value; value)
		{
			std::destroy_at(value);
			m_node_pool.deallocate(value);
		}
		std::destroy_at(node);
		m_node_pool.deallocate(node);
	}
	bool config_registry::erase(entry_ptr<true> where)
	{
		if (auto target = m_entries.find(where->m_node); target != m_entries.end()) [[likely]]
		{
			erase_impl(target);
			return true;
		}
		else
			return false;
	}
	bool config_registry::erase(const cfg_path &path)
	{
		if (auto target = m_entries.find(path.string()); target != m_entries.end()) [[likely]]
		{
			erase_impl(target);
			return true;
		}
		else
			return false;
	}
}	 // namespace sek::engine
