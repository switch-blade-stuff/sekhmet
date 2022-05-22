//
// Created by switchblade on 2022-04-24.
//

#pragma once

#include <vector>

#include "define.h"
#include "event.hpp"
#include "static_string.hpp"
#include "version.hpp"

namespace sek
{
	namespace detail
	{
		struct plugin_info
		{
			consteval plugin_info(version eng_ver, std::string_view id) noexcept : engine_ver(eng_ver), id(id) {}

			/** Version of the engine the plugin was compiled for. */
			const version engine_ver;
			/** Id of the plugin. */
			const std::string_view id;
		};
		struct plugin_data
		{
			enum status_t
			{
				INITIAL,
				DISABLED,
				ENABLED,
			};

			SEK_API static void load(plugin_data *data, void (*init)(void *));
			SEK_API static void unload(plugin_data *data);

			explicit plugin_data(plugin_info info) noexcept : info(info) {}

			[[nodiscard]] bool enable() const
			{
				bool result;
				on_enable.dispatch([&result](bool b) { return (result = b); });
				return result;
			}
			void disable() const { on_disable(); }

			/** Compile-time information about this plugin. */
			const plugin_info info;
			/** Event dispatched when a plugin is enabled by the engine. */
			event<bool(void)> on_enable;
			/** Event dispatched when a plugin is disabled by the engine. */
			event<void(void)> on_disable;

			status_t status;
		};

		template<typename Child>
		class basic_plugin : plugin_data
		{
			static Child instance;

		public:
			explicit basic_plugin(plugin_info info) noexcept : plugin_data(info)
			{
				plugin_data::load(this, [](void *p) { static_cast<Child *>(p)->init(); });
			}

			using plugin_data::info;
			using plugin_data::on_disable;
			using plugin_data::on_enable;
		};
	}	 // namespace detail

	/** @brief Handle used to reference and manage plugins. */
	class plugin
	{
	public:
		/** Returns a vector of all currently loaded plugins. */
		SEK_API static std::vector<plugin> get_loaded();
		/** Returns a vector of all currently enabled plugins. */
		SEK_API static std::vector<plugin> get_enabled();

		/** Returns a plugin using it's id. If such plugin does not exist, returns an empty handle. */
		SEK_API static plugin get(std::string_view id);

	private:
		constexpr explicit plugin(detail::plugin_data *data) noexcept : data(data) {}

	public:
		/** Initializes an empty plugin handle. */
		constexpr plugin() noexcept = default;

		/** Checks if the plugin handle is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return data == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns id of the plugin. */
		[[nodiscard]] constexpr std::string_view id() const noexcept { return data->info.id; }
		/** Returns engine version of the plugin. */
		[[nodiscard]] constexpr version engine_ver() const noexcept { return data->info.engine_ver; }

		/** Checks if the plugin is enabled. */
		[[nodiscard]] SEK_API bool enabled() const noexcept;
		/** Enables the plugin and invokes it's `on_enable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to enable if it is already enabled or not loaded or if `on_enable` returned false or threw an exception. */
		[[nodiscard]] SEK_API bool enable() const noexcept;
		/** Disables the plugin and invokes it's `on_disable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to disable if it is not enabled or not loaded. */
		[[nodiscard]] SEK_API bool disable() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const plugin &) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const plugin &) const noexcept = default;

	private:
		detail::plugin_data *data = nullptr;
	};
}	 // namespace sek

/** @brief Macro used to define a plugin.
 * @param id Unique id for the plugin used to reference the plugin at runtime. */
#define SEK_PLUGIN(id)                                                                                                 \
	namespace                                                                                                          \
	{                                                                                                                  \
		static_assert(SEK_ARRAY_SIZE(id), "Plugin id must not be empty");                                              \
                                                                                                                       \
		template<sek::basic_static_string>                                                                             \
		struct plugin_instance;                                                                                        \
		template<>                                                                                                     \
		struct plugin_instance<(id)> : sek::detail::basic_plugin<plugin_instance<(id)>>                                \
		{                                                                                                              \
			plugin_instance() : basic_plugin({sek::version{SEK_ENGINE_VERSION}, (id)})                                 \
			{                                                                                                          \
			}                                                                                                          \
			void init();                                                                                               \
		};                                                                                                             \
	}                                                                                                                  \
	template<>                                                                                                         \
	plugin_instance<(id)> sek::detail::basic_plugin<plugin_instance<(id)>>::instance = {};                             \
	void plugin_instance<(id)>::init()
