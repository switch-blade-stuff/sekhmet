//
// Created by switchblade on 2022-01-26.
//

#include "plugin.hpp"

#include <mutex>
#include <vector>

#include "hmap.hpp"

namespace sek::detail
{
	struct plugin_db
	{
		static plugin_db &get() noexcept
		{
			constinit static plugin_db instance = {};
			return instance;
		}

		std::mutex table_mtx;
		hmap<std::string_view, plugin::plugin_db_entry> plugin_table;
	};

	void plugin::register_plugin(const plugin *p) noexcept
	{
		SEK_ASSERT(p != nullptr);

		auto &db = detail::plugin_db::get();
		std::lock_guard<std::mutex> l(db.table_mtx);

		db.plugin_table.try_emplace(p->name, status_t::INITIAL, p);
	}
	void plugin::drop_plugin(const plugin *p) noexcept
	{
		SEK_ASSERT(p != nullptr);

		auto &db = detail::plugin_db::get();
		std::lock_guard<std::mutex> l(db.table_mtx);

		auto entry_iterator = db.plugin_table.find(p->name);
		if (entry_iterator != db.plugin_table.end()) [[likely]]
		{
			disable(handle{&entry_iterator->second});
			db.plugin_table.erase(entry_iterator);
		}
	}

	void plugin::enable(handle h) noexcept
	{
		if (h.empty()) [[unlikely]]
			return;

		auto expected_status = status_t::DISABLED;
		if (h.entry->status.compare_exchange_strong(expected_status, status_t::ENABLED)) [[likely]]
			h.entry->plugin_ptr->invoke_enable_queue();
	}
	void plugin::disable(handle h) noexcept
	{
		if (h.empty()) [[unlikely]]
			return;

		auto expected_status = status_t::ENABLED;
		if (h.entry->status.compare_exchange_strong(expected_status, status_t::DISABLED)) [[likely]]
			h.entry->plugin_ptr->invoke_disable_queue();
	}

	std::vector<plugin::handle> plugin::all()
	{
		auto &db = detail::plugin_db::get();
		std::lock_guard<std::mutex> l(db.table_mtx);

		std::vector<handle> result;
		result.resize(db.plugin_table.size());
		for (auto result_iter = result.begin(); auto &entry_pair : db.plugin_table)
			*result_iter++ = handle{&entry_pair.second};

		return result;
	}

	plugin::handle plugin::get(std::string_view name)
	{
		auto &db = detail::plugin_db::get();
		std::lock_guard<std::mutex> l(db.table_mtx);

		if (auto entry_iterator = db.plugin_table.find(name); entry_iterator != db.plugin_table.end()) [[likely]]
			return handle{&entry_iterator->second};
		else
			return {};
	}
	plugin::handle plugin::enable(std::string_view name)
	{
		auto result = get(name);
		enable(result);
		return result;
	}
	plugin::handle plugin::disable(std::string_view name)
	{
		auto result = get(name);
		disable(result);
		return result;
	}
}	 // namespace sek::detail
