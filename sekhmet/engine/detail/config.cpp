/*
 * Created by switchblade on 07/07/22
 */

#include "config.hpp"

#include <fstream>
#include <utility>

#include <fmt/format.h>

template class SEK_API_EXPORT sek::service<sek::shared_guard<sek::engine::config_registry>>;

namespace sek::engine
{
	using namespace serialization;

	config_error::~config_error() = default;

	void cfg_path::parse()
	{
		/* There is always at least 1 element (the category). */
		m_slices.clear();
		m_slices.reserve(1);

		const auto loc = std::locale{};
		for (std::size_t base = 0, next = 0; next < m_value.size();)
		{
			if (const auto c = m_value[next]; c == '/') [[unlikely]]
			{
				/* Strip terminating & repeating slashes. */
				if (next + 1 == m_value.size()) [[unlikely]]
					break;
				else if (m_value[next + 1] == c) [[unlikely]]
				{
					m_value.erase(next);
					continue;
				}
				base = ++next;
				m_slices.emplace_back(slice_t{next, next});
			}
			else
			{
				/* Always insert the first element. */
				if (next == 0) [[unlikely]]
					m_slices.emplace_back(slice_t{base, next});
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
			result.m_value.append(m_value.data() + slice.first, m_value.data() + slice.last);

			if (++first == last) [[unlikely]]
				break;
			result.m_value.append(1, '/');
		}
		return result;
	}

	config_registry::~config_registry() { clear_impl(); }
	void config_registry::clear()
	{
		clear_impl();
		m_node_pool.release();
		m_categories.clear();
		m_entries.clear();
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

	config_registry::entry_node *config_registry::assign_impl(entry_node *node, any &&value)
	{
		/* Always assign the value first, although we may override it during later deserialization. */
		node->value = std::forward<any>(value);
		return init_branch(node, node->data_cache);
	}
	config_registry::entry_node *config_registry::insert_impl(cfg_path &&entry, any &&value)
	{
		/* Insert, then initialize the new node. */
		auto node = insert_impl(std::forward<cfg_path>(entry));
		return assign_impl(node, std::forward<any>(value));
	}
	config_registry::entry_node *config_registry::insert_impl(cfg_path &&entry)
	{
		if (entry.empty()) [[unlikely]]
			throw config_error("Entry path cannot be empty");

		const auto add_category = entry.is_category();
		const auto node = std::construct_at(m_node_pool.allocate(), std::forward<cfg_path>(entry));
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

	config_registry::entry_node *config_registry::init_branch(entry_node *node, json_tree *cache)
	{
		/* Stack used to unwind serialization functions. Stack is pre-allocated
		 * to be able to hold all entry nodes within the current branch. */
		auto *child_path = &node->path;
		auto *parent = node;

		std::vector<entry_node *> read_stack;
		read_stack.reserve(child_path->elements());
		read_stack.push_back(node);

		/* Unwind & fill "dummy" entries for every parent path until we hit a parent with a cache. */
		while (!child_path->is_category())
		{
			/* Find or create parent node. */
			auto parent_path = child_path->parent_path();
			if (auto parent_ptr = find(parent_path); !parent_ptr)
				parent = insert_impl(std::move(parent_path));
			else
				parent = parent_ptr.m_ref.m_node;

			/* Add the child node to the parent's nodes set & push the parent to the read stack. */
			parent->nodes.insert(read_stack.back());
			child_path = &parent->path;

			/* If we still need a cache, push the read stack & try parent's cache. */
			if (!cache) [[likely]]
			{
				read_stack.emplace_back(parent);
				cache = parent->data_cache;
			}
		}

		/* If we found an upstream cache, use it to deserialize the value. */
		if (cache != nullptr) [[unlikely]]
		{
			input_archive archive{*cache};
			archive.read(*read_stack.front(), read_stack, *this);
		}

		return node;
	}

	void config_registry::erase_impl(typename entry_set::const_iterator where)
	{
		const auto node = *where;

		/* Erase all child nodes. */
		for (auto child = node->nodes.end(), end = node->nodes.begin(); child-- != end;)
			erase_impl(m_entries.find(*child));

		/* Erase the node from the node table & category list. */
		if (auto iter = m_categories.find(node); iter != m_categories.end()) [[unlikely]]
			m_categories.erase(iter);
		m_entries.erase(where);

		/* Destroy the node. */
		std::destroy_at(node);
		m_node_pool.deallocate(node);
	}
	void config_registry::clear_impl()
	{
		/* Destroy all category entries. */
		for (auto cat = m_categories.end(), end = m_categories.begin(); cat-- != end;) std::destroy_at(*cat);
	}

	bool config_registry::erase(entry_ptr<true> which)
	{
		if (!which) [[unlikely]]
			return false;

		if (auto target = m_entries.find(which->m_node); target != m_entries.end()) [[likely]]
		{
			erase_impl(target);
			return true;
		}
		else
			return false;
	}
	bool config_registry::erase(const cfg_path &entry)
	{
		if (auto target = m_entries.find(entry.string()); target != m_entries.end()) [[likely]]
		{
			erase_impl(target);
			return true;
		}
		else
			return false;
	}

	struct config_registry::entry_node::nodes_proxy
	{
		constexpr explicit nodes_proxy(std::vector<entry_node *> &stack) noexcept : stack(&stack) {}
		constexpr explicit nodes_proxy(entry_set &mut_nodes) noexcept : mut_nodes(&mut_nodes) {}
		constexpr explicit nodes_proxy(const entry_set &nodes) noexcept : nodes(&nodes) {}

		void deserialize(input_frame &f, const config_registry &r) const
		{
			/* Read the next entry with the current path. */
			auto &next = *stack->back();
			f.read(keyed_entry(next.path.entry_name().string(), next), *stack, r);
		}
		void deserialize(input_frame &f, const cfg_path &parent_path, const config_registry &r) const
		{
			/* Create a buffer for entry paths, save the size of the buffer & restore the size on every iteration. */
			auto entry_path = parent_path.string();
			entry_path.append(1, '/');
			const auto size = entry_path.size();

			for (auto entry = f.begin(), end = f.end(); entry != end; ++entry)
			{
				entry_path.resize(size);
				entry_path.append(entry.key());
				if (auto child = mut_nodes->find(entry_path); child != mut_nodes->end()) [[likely]]
					entry->read(**child, r);
			}
		}
		void serialize(output_frame &f, const config_registry &r) const
		{
			/* Serialize every child as a keyed entry, where the key is the entry name (last element) of the path. */
			for (auto *c : *nodes) f.write(serialization::keyed_entry(c->path.entry_name().string(), *c), r);
		}

		union
		{
			std::vector<entry_node *> *stack;
			const entry_set *nodes;
			entry_set *mut_nodes;
		};
	};
	struct config_registry::entry_node::any_proxy
	{
		constexpr explicit any_proxy(any &mut_value) noexcept : mut_value(&mut_value) {}
		constexpr explicit any_proxy(const any &value) noexcept : value(&value) {}

		[[nodiscard]] auto &attribute() const
		{
			return mut_value->type().get_attribute<attributes::config_type>().cast<const attributes::config_type &>();
		}
		void deserialize(input_frame &f, const config_registry &r) const { attribute().deserialize(*mut_value, f, r); }
		void serialize(output_frame &f, const config_registry &r) const { attribute().serialize(*value, f, r); }

		union
		{
			const any *value;
			any *mut_value;
		};
	};

	void config_registry::entry_node::serialize(output_frame &f, const config_registry &r) const
	{
		/* If there is a value for this entry, serialize it using the attribute. */
		if (!value.empty()) f.write(serialization::keyed_entry(value.type().name(), any_proxy{value}), r);

		/* Serialize children nodes. */
		f.write(serialization::keyed_entry("nodes", nodes_proxy{nodes}), r);
	}
	void config_registry::entry_node::deserialize(input_frame &f, const config_registry &r)
	{
		for (auto entry = f.begin(), end = f.end(); entry != end; ++entry)
		{
			if (!entry.has_key()) [[unlikely]]
				continue;

			if (const auto key = entry.key(); key == "nodes")
				entry->read(nodes_proxy{nodes}, path, r);
			else if (const auto type = type_info::get(key); type) /* Key holds the entry type. */
			{
				/* Initialize new instance if the value is empty or of an incompatible type. */
				if (value.empty() || (value.type() != type && !value.type().inherits(type))) [[unlikely]]
					value = type.construct();
				entry->read(any_proxy{value}, r);
			}
			/* Invalid key or the specified type is not reflected. */
		}
	}
	void config_registry::entry_node::deserialize(input_frame &f, std::vector<entry_node *> &s, const config_registry &r)
	{
		/* If the stack contains only 1 element, we are reading the said element.
		 * Forward to the regular deserialize function. */
		if (s.size() == 1) [[unlikely]]
			deserialize(f, r);
		else
		{
			/* Otherwise, the target node is somewhere down the stack, pop the first element off the stack,
			 * skip the "nodes" table entry through a proxy and keep on unwinding the stack. */
			s.pop_back();
			f.read(serialization::keyed_entry("nodes", nodes_proxy{s}), r);
		}
	}

	config_registry::entry_ptr<false> config_registry::load(cfg_path entry, const std::filesystem::path &path, bool cache)
	{
		auto cfg_file = std::ifstream{path};
		if (!cfg_file.is_open()) [[unlikely]]
			throw config_error(fmt::format("Failed to open config file \"{}\"", path.c_str()));

		input_archive input{cfg_file};
		return load(std::move(entry), std::move(*input.tree), cache);
	}
	config_registry::entry_ptr<false> config_registry::load(cfg_path entry, const uri &location, bool cache)
	{
		if (location.is_local()) [[likely]]
			return load(std::move(entry), std::filesystem::path{location.path(uri_format::DECODE_ALL)}, cache);
		else /* TODO: Implement non-local config loading. */
			throw config_error("Loading configuration from a non-local file is not supported yet");
	}
	config_registry::entry_ptr<false> config_registry::load(cfg_path entry, json_tree &&tree, bool cache)
	{
		/* Handle empty entry paths. */
		if (entry.empty()) [[unlikely]]
			return {};

		/* Find or create the entry node. */
		entry_node *node;
		if (auto node_ptr = find(entry); !node_ptr)
			node = insert_impl(std::move(entry));
		else
			node = node_ptr.m_ref.m_node;

		/* If there is a need to cache the Json tree, allocate new cache or swap. */
		json_tree *data = std::addressof(tree);
		if (cache) [[likely]]
		{
			if (!node->data_cache) [[likely]]
				data = node->data_cache = new json_tree{std::forward<json_tree>(tree)};
			else
				(data = node->data_cache)->swap(tree);
		}

		/* Initialize the node's branch. */
		return entry_ptr<false>{init_branch(node, data)};
	}

	bool config_registry::save(entry_ptr<true> which, const std::filesystem::path &path) const
	{
		auto file = std::ofstream{path, std::ios::trunc | std::ios::out};
		if (!file.is_open()) [[unlikely]]
			throw config_error(fmt::format("Failed to open config file \"{}\"", path.c_str()));

		output_archive archive{file};
		return save_impl(which, archive);
	}
	bool config_registry::save(entry_ptr<true> which, const uri &location) const
	{
		if (location.is_local()) [[likely]]
			return save(which, std::filesystem::path{location.path(uri_format::DECODE_ALL)});
		else /* TODO: Implement non-local config saving. */
			throw config_error("Saving configuration to a non-local file is not supported yet");
	}
	bool config_registry::save(entry_ptr<true> which, json_tree &tree) const
	{
		output_archive archive{tree};
		return save_impl(which, archive);
	}
	bool config_registry::save_impl(entry_ptr<true> which, output_archive &archive) const
	{
		if (which) [[likely]]
		{
			archive.write(*which->m_node, *this);
			return true;
		}
		return false;
	}
}	 // namespace sek::engine
