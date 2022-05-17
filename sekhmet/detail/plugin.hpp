//
// Created by switchblade on 2022-04-24.
//

#pragma once

#include <vector>

#include "define.h"
#include "event.hpp"
#include <string_view>

namespace sek
{
	namespace detail
	{
		struct basic_plugin
		{
			explicit basic_plugin(std::string_view id) noexcept : id(id) {}

			/** Id of the plugin. */
			const std::string_view id;

			/** Event dispatched when a plugin is enabled by the engine. */
			event<bool(void)> on_enable;
			/** Event dispatched when a plugin is disabled by the engine. */
			event<void(void)> on_disable;
		};
	}	 // namespace detail

	/** @brief Handle used to reference and manage plugins. */
	class plugin
	{
		/* Implemented in plugin.cpp */
		struct plugin_entry;
		struct plugin_db;

		SEK_API static void load(std::string_view, const detail::basic_plugin *) noexcept;
		SEK_API static void unload(std::string_view) noexcept;

		template<typename T>
		struct registrar
		{
			static registrar instance;

			constexpr registrar() noexcept { load(data.id, &data); }
			constexpr ~registrar() { unload(data.id); }

		private:
			const T data{};
		};

		template<typename>
		friend struct registrar;

	public:
		/** Returns a vector of all currently loaded plugins. */
		SEK_API static std::vector<plugin> get_loaded();
		/** Returns a vector of all currently enabled plugins. */
		SEK_API static std::vector<plugin> get_enabled();

		/** Returns a plugin using it's id. If such plugin does not exist, returns an empty handle. */
		SEK_API static plugin get(std::string_view id);

	private:
		constexpr explicit plugin(plugin_entry *entry) noexcept : entry(entry) {}

	public:
		/** Initializes an empty plugin handle. */
		constexpr plugin() noexcept = default;

		/** Checks if the plugin handle is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return entry == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Checks if the plugin is enabled. */
		[[nodiscard]] SEK_API bool enabled() const noexcept;
		/** Returns id of the plugin. */
		[[nodiscard]] SEK_API std::string_view id() const noexcept;

		/** Enables the plugin and invokes it's `on_enable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to enable if it is already enabled or if `on_enable` returned false or threw an exception. */
		[[nodiscard]] SEK_API bool enable() const noexcept;
		/** Disables the plugin and invokes it's `on_disable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to disable if it is not enabled. */
		[[nodiscard]] SEK_API bool disable() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const plugin &) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const plugin &) const noexcept = default;

	private:
		plugin_entry *entry = nullptr;
	};
}	 // namespace sek

#define SEK_PLUGIN_TYPE(id) sekhmet_plugins_##id

/** @brief Macro used to define a plugin.
 * @param type_name Unique id for the plugin used to reference the plugin at runtime.
 * @note Id must be a valid type name. */
#define SEK_PLUGIN(id)                                                                                                 \
	namespace                                                                                                          \
	{                                                                                                                  \
		static_assert(SEK_ARRAY_SIZE(#id), "Plugin id must not be empty");                                             \
		struct SEK_PLUGIN_TYPE(id) : sek::detail::basic_plugin                                                         \
		{                                                                                                              \
			SEK_PLUGIN_TYPE(id)();                                                                                     \
		};                                                                                                             \
	}                                                                                                                  \
	template<>                                                                                                         \
	sek::plugin::registrar<SEK_PLUGIN_TYPE(id)> sek::plugin::registrar<SEK_PLUGIN_TYPE(id)>::instance = {};            \
	SEK_PLUGIN_TYPE(id)::SEK_PLUGIN_TYPE(id)() : sek::detail::basic_plugin(#id)
