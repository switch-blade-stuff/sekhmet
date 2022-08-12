/*
 * Created by switchblade on 28/04/22
 */

#include "plugin.hpp"

#include "sekhmet/dense_map.hpp"

#include "logger.hpp"
#include <shared_mutex>

#define ENABLE_FAIL_MSG "Failed to enable plugin - "
#define DISABLE_FAIL_MSG "Failed to disable plugin - "

namespace sek::engine
{
	namespace detail
	{
		struct plugin_db
		{
			static plugin_db instance;

			void filter(auto &&f) const
			{
				std::shared_lock<std::shared_mutex> l(mtx);
				for (auto entry : plugins) f(entry.second);
			}

			mutable std::shared_mutex mtx;
			dense_map<std::string_view, plugin_data *> plugins;
		};

		plugin_db plugin_db::instance;

		static auto format_plugin(const plugin_data *data)
		{
			return fmt::format("\"{}\" ({})", data->info.id, data->info.plugin_ver.to_string());
		}

		static bool is_compatible(const version &a, const version &b) noexcept
		{
			return a.major() == b.major() && a.minor() <= b.minor();
		}
		static bool is_compatible(const version &ver) noexcept
		{
			return is_compatible(ver, version{SEK_ENGINE_VERSION});
		}
		static bool enable_guarded(plugin_data *data) noexcept
		{
			try
			{
				return data->enable();
			}
			catch (std::runtime_error &e)
			{
				logger::error() << fmt::format(ENABLE_FAIL_MSG "got runtime error: \"{}\"", e.what());
			}
			catch (std::exception &e)
			{
				logger::error() << fmt::format(ENABLE_FAIL_MSG "got exception: \"{}\". This my lead to a crash", e.what());
			}
			catch (...)
			{
				logger::error() << fmt::format(ENABLE_FAIL_MSG "unknown exception. This my lead to a crash");
			}
			return false;
		}
		static void disable_guarded(plugin_data *data) noexcept
		{
			try
			{
				data->disable();
			}
			catch (std::runtime_error &e)
			{
				logger::error() << fmt::format(DISABLE_FAIL_MSG "got runtime error: \"{}\"", e.what());
			}
			catch (std::exception &e)
			{
				logger::error() << fmt::format(DISABLE_FAIL_MSG "got exception: \"{}\". This my lead to a crash", e.what());
			}
			catch (...)
			{
				logger::error() << fmt::format(DISABLE_FAIL_MSG "unknown exception. This my lead to a crash");
			}
		}

		void plugin_data::load_impl(plugin_data *data, void (*init)(void *))
		{
			auto &db = plugin_db::instance;

			if (const auto ver = data->info.engine_ver; !is_compatible(ver)) [[unlikely]]
			{
				logger::error() << fmt::format("Ignoring incompatible plugin {}. "
											   "Plugin's engine version: \"{}\", "
											   "actual engine version: \"{}\"",
											   format_plugin(data),
											   data->info.engine_ver.to_string(),
											   SEK_ENGINE_VERSION);
				return;
			}
			else if (data->status != plugin_data::INITIAL) [[unlikely]]
			{
				logger::error() << fmt::format("Ignoring plugin {} - already loaded", format_plugin(data));
				return;
			}
			else
			{
				bool auto_enable = false;
				const auto existing = db.plugins.try_emplace(data->info.id, data);
				if (!existing.second) [[unlikely]]
				{
					if (auto existing_ver = existing.first->second->info.plugin_ver; is_compatible(existing_ver, ver))
					{
						logger::warn() << fmt::format("Ignoring plugin {} - lesser version", format_plugin(data));
						return;
					}
					else /* Replace existing plugin. */
					{
						logger::info() << fmt::format("Replacing plugin {} with "
													  "greater version number ({}) ",
													  format_plugin(data),
													  ver.to_string());

						if ((auto_enable = existing.first->second->status == plugin_data::ENABLED))
							disable_guarded(existing.first->second);
						unload_impl(existing.first->second);
					}
				}

				logger::info() << fmt::format("Loading plugin {}", format_plugin(data));
				try
				{
					init(data);
					data->status = plugin_data::DISABLED;
					if (auto_enable && !enable_guarded(data)) [[unlikely]]
						logger::warn() << fmt::format("Failed to enable replacement plugin");
					return;
				}
				catch (std::exception &e)
				{
					logger::error() << fmt::format("Failed to load plugin - init exception: \"{}\"", e.what());
				}
				catch (...)
				{
					logger::error() << fmt::format("Failed to load plugin - unknown init exception");
				}
				db.plugins.erase(existing.first);
			}
		}
		void plugin_data::unload_impl(plugin_data *data)
		{
			auto &db = plugin_db::instance;

			const auto old_status = std::exchange(data->status, plugin_data::INITIAL);
			if (old_status == plugin_data::INITIAL) [[unlikely]]
				return;

			logger::info() << fmt::format("Unloading plugin {}", format_plugin(data));
			if (old_status == plugin_data::ENABLED) [[unlikely]]
			{
				logger::warn() << fmt::format("Plugin is still enabled. Disabling on unload. "
											  "This may lead to unexpected errors",
											  data->info.id);
				disable_guarded(data);
			}

			db.plugins.erase(data->info.id);
		}
		void plugin_data::load(plugin_data *data, void (*init)(void *))
		{
			std::lock_guard<std::shared_mutex> l(plugin_db::instance.mtx);
			load_impl(data, init);
		}
		void plugin_data::unload(plugin_data *data)
		{
			std::lock_guard<std::shared_mutex> l(plugin_db::instance.mtx);
			unload_impl(data);
		}
	}	 // namespace detail

	std::vector<plugin> plugin::get_loaded()
	{
		std::vector<plugin> result;
		detail::plugin_db::instance.filter([&result](auto *ptr) { result.push_back(plugin{ptr}); });
		return result;
	}
	std::vector<plugin> plugin::get_enabled()
	{
		std::vector<plugin> result;
		detail::plugin_db::instance.filter(
			[&result](auto *ptr)
			{
				if (ptr->status == detail::plugin_data::ENABLED) result.push_back(plugin{ptr});
			});
		return result;
	}
	plugin plugin::get(std::string_view id)
	{
		auto &db = detail::plugin_db::instance;
		std::shared_lock<std::shared_mutex> l(db.mtx);

		if (auto pos = db.plugins.find(id); pos != db.plugins.end()) [[likely]]
			return plugin{pos->second};
		return plugin{};
	}

	bool plugin::enabled() const noexcept
	{
		std::shared_lock<std::shared_mutex> l(detail::plugin_db::instance.mtx);
		return m_data->status == detail::plugin_data::ENABLED;
	}
	bool plugin::enable() const noexcept
	{
		std::lock_guard<std::shared_mutex> l(detail::plugin_db::instance.mtx);

		logger::info() << fmt::format("Enabling plugin {}", detail::format_plugin(m_data));
		if (m_data->status != detail::plugin_data::DISABLED) [[unlikely]]
			logger::error() << fmt::format(ENABLE_FAIL_MSG "already enabled or not loaded");
		else if (detail::enable_guarded(m_data)) [[likely]]
		{
			m_data->status = detail::plugin_data::ENABLED;
			return true;
		}
		return false;
	}
	bool plugin::disable() const noexcept
	{
		std::lock_guard<std::shared_mutex> l(detail::plugin_db::instance.mtx);

		logger::info() << fmt::format("Disabling plugin {}", detail::format_plugin(m_data));
		if (m_data->status != detail::plugin_data::ENABLED) [[unlikely]]
			logger::error() << fmt::format(DISABLE_FAIL_MSG "already disabled or not loaded");
		else
		{
			detail::disable_guarded(m_data);
			m_data->status = detail::plugin_data::DISABLED;
			return true;
		}
		return false;
	}
}	 // namespace sek::engine
