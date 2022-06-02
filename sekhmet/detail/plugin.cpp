/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 28/04/22
 */

#include "plugin.hpp"

#include "dense_map.hpp"
#include "logger.hpp"
#include <shared_mutex>

#define ENABLE_FAIL_MSG "Failed to enable plugin - "
#define DISABLE_FAIL_MSG "Failed to disable plugin - "

namespace sek
{
	namespace detail
	{
		struct plugin_db
		{
			static plugin_db &instance()
			{
				static plugin_db value;
				return value;
			}

			void filter(auto &&f) const
			{
				std::shared_lock<std::shared_mutex> l(mtx);
				for (auto entry : plugins) f(entry.second);
			}

			mutable std::shared_mutex mtx;
			dense_map<std::string_view, plugin_data *> plugins;
		};

		static bool check_version(const version &ver) noexcept
		{
			const auto engine_ver = version{SEK_ENGINE_VERSION};
			return ver.major() == engine_ver.major() && ver.minor() <= engine_ver.minor();
		}
		static bool enable_guarded(plugin_data *data) noexcept
		{
			try
			{
				return data->enable();
			}
			catch (std::exception &e)
			{
				logger::error() << SEK_LOG_FORMAT_NS::format(ENABLE_FAIL_MSG "got exception: \"{}\"", e.what());
			}
			catch (...)
			{
				logger::error() << SEK_LOG_FORMAT_NS::format(ENABLE_FAIL_MSG "unknown exception");
			}
			return false;
		}
		static void disable_guarded(plugin_data *data) noexcept
		{
			try
			{
				data->disable();
			}
			catch (std::exception &e)
			{
				logger::error() << SEK_LOG_FORMAT_NS::format(DISABLE_FAIL_MSG "got exception: \"{}\"", e.what());
				return;
			}
			catch (...)
			{
				logger::error() << SEK_LOG_FORMAT_NS::format(DISABLE_FAIL_MSG "unknown exception");
				return;
			}
		}

		void plugin_data::load(plugin_data *data, void (*init)(void *))
		{
			auto &db = plugin_db::instance();
			std::lock_guard<std::shared_mutex> l(db.mtx);

			if (!check_version(data->info.engine_ver)) [[unlikely]]
				logger::error() << SEK_LOG_FORMAT_NS::format("Ignoring incompatible plugin \"{}\". "
															 "Plugin engine version: \"{}\", "
															 "actual engine version: \"{}\"",
															 data->info.id,
															 data->info.engine_ver.to_string(),
															 SEK_ENGINE_VERSION);
			else if (data->status != plugin_data::INITIAL) [[unlikely]]
				logger::error() << SEK_LOG_FORMAT_NS::format("Ignoring duplicate plugin \"{}\"", data->info.id);
			else if (auto res = db.plugins.try_emplace(data->info.id, data); res.second) [[likely]]
			{
				logger::info() << SEK_LOG_FORMAT_NS::format("Loading plugin \"{}\"", data->info.id);

				try
				{
					init(data);
					data->status = plugin_data::DISABLED;
					return;
				}
				catch (std::exception &e)
				{
					logger::error() << SEK_LOG_FORMAT_NS::format("Failed to load plugin - init exception: \"{}\"", e.what());
				}
				catch (...)
				{
					logger::error() << SEK_LOG_FORMAT_NS::format("Failed to load plugin - unknown init exception");
				}
				db.plugins.erase(res.first);
			}
			else
				logger::warn() << SEK_LOG_FORMAT_NS::format("Ignoring duplicate plugin \"{}\"", data->info.id);
		}
		void plugin_data::unload(plugin_data *data)
		{
			auto &db = plugin_db::instance();
			std::lock_guard<std::shared_mutex> l(db.mtx);

			const auto old_status = std::exchange(data->status, plugin_data::INITIAL);
			if (old_status == plugin_data::INITIAL) [[unlikely]]
				return;

			logger::info() << SEK_LOG_FORMAT_NS::format("Unloading plugin \"{}\"", data->info.id);
			if (old_status == plugin_data::ENABLED) [[unlikely]]
			{
				logger::warn() << SEK_LOG_FORMAT_NS::format("Disabling plugin \"{}\" on unload. "
															"This may lead to unexpected errors",
															data->info.id);
				disable_guarded(data);
			}

			db.plugins.erase(data->info.id);
		}
	}	 // namespace detail

	std::vector<plugin> plugin::get_loaded()
	{
		std::vector<plugin> result;
		detail::plugin_db::instance().filter([&result](auto *ptr) { result.push_back(plugin{ptr}); });
		return result;
	}
	std::vector<plugin> plugin::get_enabled()
	{
		std::vector<plugin> result;
		detail::plugin_db::instance().filter(
			[&result](auto *ptr)
			{
				if (ptr->status == detail::plugin_data::ENABLED) result.push_back(plugin{ptr});
			});
		return result;
	}
	plugin plugin::get(std::string_view id)
	{
		auto &db = detail::plugin_db::instance();
		std::shared_lock<std::shared_mutex> l(db.mtx);

		if (auto pos = db.plugins.find(id); pos != db.plugins.end()) [[likely]]
			return plugin{pos->second};
		return plugin{};
	}

	bool plugin::enabled() const noexcept
	{
		std::shared_lock<std::shared_mutex> l(detail::plugin_db::instance().mtx);
		return data->status == detail::plugin_data::ENABLED;
	}
	bool plugin::enable() const noexcept
	{
		std::lock_guard<std::shared_mutex> l(detail::plugin_db::instance().mtx);

		logger::info() << SEK_LOG_FORMAT_NS::format("Enabling plugin \"{}\"", id());
		if (data->status != detail::plugin_data::DISABLED) [[unlikely]]
			logger::error() << SEK_LOG_FORMAT_NS::format(ENABLE_FAIL_MSG "already enabled or not loaded");
		else if (detail::enable_guarded(data)) [[likely]]
		{
			data->status = detail::plugin_data::ENABLED;
			return true;
		}
		return false;
	}
	bool plugin::disable() const noexcept
	{
		std::lock_guard<std::shared_mutex> l(detail::plugin_db::instance().mtx);

		logger::info() << SEK_LOG_FORMAT_NS::format("Disabling plugin \"{}\"", id());
		if (data->status != detail::plugin_data::ENABLED) [[unlikely]]
			logger::error() << SEK_LOG_FORMAT_NS::format(DISABLE_FAIL_MSG "already disabled or not loaded");
		else
		{
			detail::disable_guarded(data);
			data->status = detail::plugin_data::DISABLED;
			return true;
		}
		return false;
	}
}	 // namespace sek
