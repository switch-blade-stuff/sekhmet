//
// Created by switchblade on 28/04/22.
//

#include "plugin.hpp"

#include <atomic>
#include <mutex>

#include "logger.hpp"
#include "sparse_map.hpp"

#define ENABLE_FAIL_MSG "Failed to enable plugin \"{}\". "

namespace sek
{
	struct plugin::plugin_entry
	{
		enum status_t : int
		{
			DISABLED,
			ENABLED,
		};

		constexpr explicit plugin_entry(const detail::basic_plugin *data) noexcept : data(data) {}

		plugin_entry() noexcept = default;
		plugin_entry(plugin_entry &&other) noexcept : data(other.data), status(other.status.load()) {}

		[[nodiscard]] std::string_view id() const noexcept { return data->id; }
		[[nodiscard]] bool enable() noexcept
		{
			try
			{
				bool result = true;
				data->on_enable.dispatch([&result](bool b) { return result = result || b; });

				if (!result) [[unlikely]]
					logger::error() << fmt::format(ENABLE_FAIL_MSG "`on_enable` event returned false", id());
				else
				{
					logger::info() << fmt::format("Plugin \"{}\" enabled successfully", id());
					return true;
				}
			}
			catch (std::exception &e)
			{
				logger::error() << fmt::format(ENABLE_FAIL_MSG "Got exception: {}", id(), e.what());
			}
			catch (...)
			{
				logger::error() << fmt::format(ENABLE_FAIL_MSG "Unknown exception", id());
			}
			status = plugin_entry::DISABLED;
			return false;
		}
		void disable() const noexcept
		{
			try
			{
				data->on_disable.dispatch();
				logger::info() << fmt::format("Plugin \"{}\" disabled successfully", id());
			}
			catch (std::exception &e)
			{
				logger::error() << fmt::format("Exception in plugin \"{}\" `on_disable` event: {}", id(), e.what());
			}
			catch (...)
			{
				logger::error() << fmt::format("Unknown exception in plugin \"{}\" `on_disable` event", id());
			}
		}

		const detail::basic_plugin *data;
		std::atomic<status_t> status = DISABLED;
	};
	struct plugin::plugin_db
	{
		std::mutex mtx;
		sparse_map<std::string_view, plugin_entry> plugins;
	};

	plugin::plugin_db &plugin::database()
	{
		static plugin_db instance;
		return instance;
	}

	void plugin::load(std::string_view id, const detail::basic_plugin *data) noexcept
	{
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		if (db.plugins.try_emplace(id, data).second) [[likely]]
			logger::info() << fmt::format("Loaded plugin \"{}\"", id);
		else
			logger::warn() << fmt::format("Attempted to load duplicate plugin \"{}\"", id);
	}
	void plugin::unload(std::string_view id) noexcept
	{
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		if (auto iter = db.plugins.find(id); iter == db.plugins.end()) [[unlikely]]
			logger::warn() << fmt::format("Attempted to unload unknown plugin \"{}\"", id);
		else
		{
			if (iter->second.status == plugin_entry::ENABLED)
			{
				logger::warn() << fmt::format("Disabling plugin \"{}\" on unload", id);
				iter->second.disable();
			}
			db.plugins.erase(iter);
			logger::info() << fmt::format("Unloaded plugin \"{}\"", id);
		}
	}

	std::vector<plugin> plugin::get_loaded()
	{
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		std::vector<plugin> result;
		result.reserve(db.plugins.size());
		for (auto &p : db.plugins) result.push_back(plugin{&p.second});

		return result;
	}
	std::vector<plugin> plugin::get_enabled()
	{
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		std::vector<plugin> result;
		result.reserve(db.plugins.size());
		for (auto &p : db.plugins)
			if (p.second.status == plugin_entry::ENABLED) result.push_back(plugin{&p.second});

		return result;
	}
	plugin plugin::get(std::string_view id)
	{
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		if (auto iter = db.plugins.find(id); iter == db.plugins.end()) [[unlikely]]
		{
			logger::error() << fmt::format("Attempted to get unknown plugin {}", id);
			return {};
		}
		else
			return plugin{&iter->second};
	}

	std::string_view plugin::id() const noexcept { return entry->id(); }
	bool plugin::enabled() const noexcept { return entry->status == plugin_entry::ENABLED; }

	bool plugin::enable() const noexcept
	{
		const auto entry_id = id();
		auto expected = plugin_entry::DISABLED;

		if (entry->status.compare_exchange_strong(expected, plugin_entry::ENABLED)) [[likely]]
		{
			logger::info() << fmt::format("Enabling plugin \"{}\"...", entry_id);
			return entry->enable();
		}
		else
		{
			logger::warn() << fmt::format("Attempted to enable already enabled plugin \"{}\"", entry_id);
			return false;
		}
	}
	bool plugin::disable() const noexcept
	{
		const auto entry_id = id();
		auto expected = plugin_entry::ENABLED;

		if (entry->status.compare_exchange_strong(expected, plugin_entry::DISABLED)) [[likely]]
		{
			logger::info() << fmt::format("Disabling plugin \"{}\"...", entry_id);
			entry->disable();
			return true;
		}
		else
		{
			logger::warn() << fmt::format("Attempted to disable already disabled plugin \"{}\"", entry_id);
			return false;
		}
	}
}	 // namespace sek
