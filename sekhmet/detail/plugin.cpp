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

		constexpr plugin_entry(const plugin_info *info, void *data) noexcept : info(info), data(data) {}

		plugin_entry() noexcept = default;
		plugin_entry(plugin_entry &&other) noexcept : info(other.info), data(other.data), status(other.status.load()) {}

		[[nodiscard]] std::string_view id() const noexcept { return info->id(data); }
		bool enable() noexcept
		{
			try
			{
				if (!info->enable(data)) [[unlikely]]
					logger::error() << fmt::format(ENABLE_FAIL_MSG "`on_enable` returned false", id());
				logger::msg() << fmt::format("Plugin \"{}\" enabled successfully", id());
				return true;
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
				info->disable(data);
				logger::msg() << fmt::format("Plugin \"{}\" disabled successfully", id());
			}
			catch (std::exception &e)
			{
				logger::error() << fmt::format("Exception in plugin \"{}\" `on_disable` function: {}", id(), e.what());
			}
			catch (...)
			{
				logger::error() << fmt::format("Unknown exception in plugin \"{}\" `on_disable` function", id());
			}
		}

		const plugin_info *info;
		void *data;
		std::atomic<status_t> status = DISABLED;
	};
	struct plugin::plugin_db
	{
		std::mutex mtx;
		sparse_map<std::string_view, plugin_entry> plugins;
	};

	SEK_API_EXPORT plugin::plugin_db &plugin::database()
	{
		static plugin_db instance;
		return instance;
	}

	SEK_API_EXPORT void plugin::load_plugin(const plugin_info *info, void *data) noexcept
	{
		const auto id = info->id(data);
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		if (db.plugins.try_emplace(id, info, data).second) [[likely]]
			logger::msg() << fmt::format("Loaded plugin \"{}\"", id);
		else
			logger::warn() << fmt::format("Attempted to load duplicate plugin \"{}\"", id);
	}
	SEK_API_EXPORT void plugin::unload_plugin(const plugin_info *info, void *data) noexcept
	{
		const auto id = info->id(data);
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
			logger::msg() << fmt::format("Unloaded plugin \"{}\"", id);
		}
	}

	SEK_API_EXPORT std::vector<plugin> plugin::get_loaded()
	{
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		std::vector<plugin> result;
		result.reserve(db.plugins.size());
		for (auto &p : db.plugins) result.push_back(plugin{&p.second});

		return result;
	}
	SEK_API_EXPORT std::vector<plugin> plugin::get_enabled()
	{
		auto &db = database();
		std::lock_guard<std::mutex> l(db.mtx);

		std::vector<plugin> result;
		result.reserve(db.plugins.size());
		for (auto &p : db.plugins)
			if (p.second.status == plugin_entry::ENABLED) result.push_back(plugin{&p.second});

		return result;
	}
	SEK_API_EXPORT plugin plugin::get(std::string_view id)
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

	SEK_API_EXPORT std::string_view plugin::id() const noexcept { return entry->id(); }
	SEK_API_EXPORT bool plugin::enabled() const noexcept { return entry->status == plugin_entry::ENABLED; }
	SEK_API_EXPORT void *plugin::data() const noexcept { return entry->data; }

	SEK_API_EXPORT bool plugin::enable() const noexcept
	{
		const auto entry_id = id();
		auto expected = plugin_entry::DISABLED;

		if (entry->status.compare_exchange_strong(expected, plugin_entry::ENABLED)) [[likely]]
		{
			logger::msg() << fmt::format("Enabling plugin \"{}\"...", entry_id);
			return entry->enable();
		}
		else
		{
			logger::warn() << fmt::format("Attempted to enable already enabled plugin \"{}\"", entry_id);
			return false;
		}
	}
	SEK_API_EXPORT bool plugin::disable() const noexcept
	{
		const auto entry_id = id();
		auto expected = plugin_entry::ENABLED;

		if (entry->status.compare_exchange_strong(expected, plugin_entry::DISABLED)) [[likely]]
		{
			logger::msg() << fmt::format("Disabling plugin \"{}\"...", entry_id);
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
